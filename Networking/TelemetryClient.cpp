#include "Networking/TelemetryClient.h"
#include <grpcpp/grpcpp.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace wire = pi::telemetry::v1;
using namespace std::chrono_literals;

TelemetryClient::TelemetryClient(const std::string& address)
    : TelemetryClient(address, Options{}) {}

TelemetryClient::TelemetryClient(const std::string& address, Options options)
    : options_(options),
      stub_(wire::Telemetry::NewStub(grpc::CreateChannel(address, grpc::InsecureChannelCredentials())))
{
    if (address.empty() || options_.staleAfter <= 0ms || options_.retryInitial <= 0ms ||
        options_.retryMaximum < options_.retryInitial)
        throw std::invalid_argument("Invalid telemetry address or timing options");
    try
    {
        workers_[Temperature] = std::thread([this] {
            subscribe<wire::TemperatureSample>(Temperature,
                [this](auto* c, const auto& r) { return stub_->StreamTemperature(c, r); },
                [](const auto& s) { return s.celsius(); });
        });
        workers_[Pressure] = std::thread([this] {
            subscribe<wire::PressureSample>(Pressure,
                [this](auto* c, const auto& r) { return stub_->StreamPressure(c, r); },
                [](const auto& s) { return s.kilopascals(); });
        });
        workers_[Waveform] = std::thread([this] {
            subscribe<wire::WaveformSample>(Waveform,
                [this](auto* c, const auto& r) { return stub_->StreamWaveform(c, r); },
                [](const auto& s) { return s.volts(); });
        });
        workers_[Monitor] = std::thread([this] {
            subscribe<wire::MonitorFrame>(Monitor,
                [this](auto* c, const auto&) { return stub_->StreamMonitor(c, wire::MonitorRequest{}); },
                [](const auto&) { return 0.0; });
        });
        watchdog_ = std::thread([this] { watchForStalls(); });
    }
    catch (...)
    {
        stop();
        throw;
    }
}

TelemetryClient::~TelemetryClient() { stop(); }

void TelemetryClient::stop()
{
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
        // Context pointers are registered/unregistered under the same lock, so
        // they remain alive for every TryCancel, including connection failures.
        for (auto* context : contexts_)
            if (context) context->TryCancel();
    }
    wake_.notify_all();
    for (auto& worker : workers_)
        if (worker.joinable()) worker.join();
    if (watchdog_.joinable()) watchdog_.join();
}

TelemetryClient::Snapshot TelemetryClient::snapshot() const
{
    std::lock_guard lock(mutex_);
    Snapshot result;
    result.readings = readings_;
    result.waveform.assign(waveform_.begin(), waveform_.end());
    result.revision = revision_;
    result.parameters = parameters_;
    for (std::size_t i = 0; i < trends_.size(); ++i)
        result.trends[i].assign(trends_[i].begin(), trends_[i].end());
    return result;
}

void TelemetryClient::clearMonitor()
{
    for (std::size_t i = 0; i < parameters_.size(); ++i) {
        parameters_[i] = {};
        parameters_[i].status = "Monitor disconnected";
        trends_[i].clear();
    }
}

std::uint64_t TelemetryClient::receivedSamples() const
{
    std::lock_guard lock(mutex_);
    return receivedSamples_;
}

bool TelemetryClient::recordMonitor(const wire::MonitorFrame& frame)
{
    if (!frame.has_metadata() || !std::isfinite(frame.metadata().elapsed_seconds())) return false;
    for (const auto& value : frame.values())
        if (value.type() < wire::CO || value.type() > wire::STO2_B2 || !std::isfinite(value.value()))
            return false;
    std::lock_guard lock(mutex_);
    if (stopping_) return false;
    lastActivity_[Monitor] = std::chrono::steady_clock::now();
    readings_[Monitor].connected = readings_[Monitor].available = true;
    readings_[Monitor].status = "Connected";
    readings_[Monitor].sequence = frame.metadata().sequence();
    std::array<bool, 8> active{};
    for (std::size_t i = 0; i < 4; ++i)
        active[i] = frame.psc_cable_connected() && frame.psc_sensor_connected();
    for (const auto type : frame.connected_sto2())
        if (type >= wire::STO2_A1 && type <= wire::STO2_B2) active[static_cast<std::size_t>(type - 1)] = true;
    for (std::size_t i = 0; i < parameters_.size(); ++i) {
        parameters_[i].connected = active[i];
        if (!active[i]) {
            parameters_[i].available = false;
            parameters_[i].status = i < 4 ? "PSC cable / sensor disconnected" : "Channel disconnected";
            trends_[i].clear();
        } else if (!parameters_[i].available) parameters_[i].status = "Waiting for reading";
    }
    for (const auto& value : frame.values()) {
        const auto i = static_cast<std::size_t>(value.type() - 1);
        if (!active[i]) continue;
        ++receivedSamples_;
        auto& reading = parameters_[i];
        reading.available = true;
        reading.value = value.value();
        reading.status = "Connected";
        reading.sequence = frame.metadata().sequence();
        const double seconds = frame.metadata().elapsed_seconds();
        auto& history = trends_[i];
        if (!history.empty() && seconds <= history.back().seconds) history.clear();
        // Ten minutes, plus a strict count cap even if a server sends too fast.
        while (!history.empty() && (history.size() >= 301 || seconds - history.front().seconds > 600))
            history.pop_front();
        history.push_back({seconds, value.value()});
    }
    ++revision_;
    return true;
}

bool TelemetryClient::record(Stream stream, double value, const wire::SampleMetadata& metadata)
{
    if (!std::isfinite(value) || !std::isfinite(metadata.elapsed_seconds())) return false;
    std::lock_guard lock(mutex_);
    if (stopping_) return false;
    auto& reading = readings_[stream];
    reading.connected = true;
    reading.available = true;
    reading.value = value;
    ++receivedSamples_;
    reading.sequence = metadata.sequence();
    reading.status = "Connected";
    lastActivity_[stream] = std::chrono::steady_clock::now();
    if (stream == Waveform)
    {
        if (waveform_.size() == waveformCapacity) waveform_.pop_front();
        waveform_.push_back({metadata.elapsed_seconds(), value});
    }
    ++revision_;
    return true;
}

template<class Sample, class Start, class Value>
void TelemetryClient::subscribe(Stream stream, Start start, Value value)
{
    auto retry = options_.retryInitial;
    for (;;)
    {
        grpc::ClientContext context;
        {
            std::lock_guard lock(mutex_);
            if (stopping_) return;
            contexts_[stream] = &context;
            lastActivity_[stream] = std::chrono::steady_clock::now();
            readings_[stream].connected = false;
            readings_[stream].available = false;
            readings_[stream].status = "Connecting";
            if (stream == Waveform) waveform_.clear();
            if (stream == Monitor) clearMonitor();
            ++revision_;
        }
        // Keep the server's defaults: temperature 1, pressure 10, waveform 100 Hz.
        wire::StreamRequest request;
        auto reader = start(&context, request);
        Sample sample;
        bool received = false;
        bool invalid = false;
        while (reader->Read(&sample))
        {
            bool valid = false;
            if constexpr (std::is_same_v<Sample, wire::MonitorFrame>) {
                valid = recordMonitor(sample);
            } else if constexpr (std::is_same_v<Sample, wire::WaveformSample>) {
                if (sample.inactive() && sample.has_metadata()) {
                    std::lock_guard lock(mutex_);
                    lastActivity_[stream] = std::chrono::steady_clock::now();
                    readings_[stream].connected = true;
                    readings_[stream].available = false;
                    readings_[stream].status = "PSC cable / sensor disconnected";
                    waveform_.clear();
                    ++revision_;
                    valid = true;
                } else valid = sample.has_metadata() && record(stream, value(sample), sample.metadata());
            } else valid = sample.has_metadata() && record(stream, value(sample), sample.metadata());
            if (!valid)
            {
                invalid = true;
                context.TryCancel();
                break;
            }
            received = true;
        }
        const auto status = reader->Finish();
        {
            std::unique_lock lock(mutex_);
            contexts_[stream] = nullptr;
            if (stopping_) return;
            readings_[stream].connected = false;
            readings_[stream].available = false;
            readings_[stream].status = invalid ? "Invalid sample; retrying" :
                status.ok() ? "Stream ended; retrying" : "Disconnected; retrying (" +
                    std::to_string(static_cast<int>(status.error_code())) + ")";
            if (stream == Waveform) waveform_.clear();
            if (stream == Monitor) clearMonitor();
            ++revision_;
            if (received) retry = options_.retryInitial;
            if (wake_.wait_for(lock, retry, [this] { return stopping_; })) return;
        }
        retry = std::min(retry * 2, options_.retryMaximum);
    }
}

void TelemetryClient::watchForStalls()
{
    std::unique_lock lock(mutex_);
    while (!wake_.wait_for(lock, 100ms, [this] { return stopping_; }))
    {
        const auto now = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < StreamCount; ++i)
        {
            if (contexts_[i] && now - lastActivity_[i] > options_.staleAfter)
            {
                readings_[i].connected = false;
                readings_[i].available = false;
                readings_[i].status = "No data; reconnecting";
                if (i == Waveform) waveform_.clear();
                if (i == Monitor) clearMonitor();
                ++revision_;
                contexts_[i]->TryCancel();
            }
        }
    }
}
