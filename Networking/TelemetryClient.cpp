#include "Networking/TelemetryClient.h"
#include <grpcpp/grpcpp.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

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
    return {readings_, {waveform_.begin(), waveform_.end()}, revision_};
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
            if (!sample.has_metadata() || !record(stream, value(sample), sample.metadata()))
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
                ++revision_;
                contexts_[i]->TryCancel();
            }
        }
    }
}
