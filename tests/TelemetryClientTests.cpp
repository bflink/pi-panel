#include "Networking/TelemetryClient.h"
#include <grpcpp/grpcpp.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace wire = pi::telemetry::v1;
using namespace std::chrono_literals;

void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}
template<class Predicate>
void eventually(Predicate predicate, const char* message)
{
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    while (!predicate())
    {
        if (std::chrono::steady_clock::now() >= deadline) throw std::runtime_error(message);
        std::this_thread::sleep_for(10ms);
    }
}

class Fixture final : public wire::Telemetry::Service
{
public:
    std::atomic<bool> stall{false};
    std::atomic<bool> dropWaveform{false};
    std::atomic<unsigned> waveformCalls{0};
    std::atomic<bool> pscActive{true};
    grpc::Status StreamMonitor(grpc::ServerContext* c, const wire::MonitorRequest*,
                               grpc::ServerWriter<wire::MonitorFrame>* w) override
    {
        std::uint64_t sequence = 0;
        bool previous = false;
        while (!c->IsCancelled()) {
            if (!stall.load()) {
                wire::MonitorFrame f;
                f.mutable_metadata()->set_sequence(sequence++);
                f.mutable_metadata()->set_elapsed_seconds(static_cast<double>(sequence) / 10);
                const bool active = pscActive.load();
                f.set_psc_cable_connected(active);
                f.set_psc_sensor_connected(active);
                if (active && !previous) {
                    auto* v = f.add_values(); v->set_type(wire::CO); v->set_value(5.2);
                }
                previous = active;
                if (!w->Write(f)) break;
            }
            std::this_thread::sleep_for(20ms);
        }
        return grpc::Status::OK;
    }

    template<class Sample, class SetValue>
    grpc::Status stream(grpc::ServerContext* c, grpc::ServerWriter<Sample>* w,
                        bool waveform, SetValue setValue)
    {
        std::uint64_t sequence = 0;
        while (!c->IsCancelled())
        {
            if (waveform && dropWaveform.load())
                return {grpc::StatusCode::UNAVAILABLE, "Test disconnect"};
            if (!stall.load())
            {
                Sample sample;
                sample.mutable_metadata()->set_sequence(sequence);
                sample.mutable_metadata()->set_elapsed_seconds(static_cast<double>(sequence) / 100.0);
                sample.mutable_metadata()->set_unix_time_ms(1700000000000LL + static_cast<std::int64_t>(sequence));
                setValue(sample);
                ++sequence;
                if (!w->Write(sample)) break;
            }
            std::this_thread::sleep_for(1ms);
        }
        return grpc::Status::OK;
    }
    grpc::Status StreamTemperature(grpc::ServerContext* c, const wire::StreamRequest*,
                                   grpc::ServerWriter<wire::TemperatureSample>* w) override
    { return stream(c, w, false, [](auto& s) { s.set_celsius(23.5); }); }
    grpc::Status StreamPressure(grpc::ServerContext* c, const wire::StreamRequest*,
                                grpc::ServerWriter<wire::PressureSample>* w) override
    { return stream(c, w, false, [](auto& s) { s.set_kilopascals(101.2); }); }
    grpc::Status StreamWaveform(grpc::ServerContext* c, const wire::StreamRequest*,
                                grpc::ServerWriter<wire::WaveformSample>* w) override
    {
        ++waveformCalls;
        return stream(c, w, true, [this](auto& s) { s.set_volts(1.65); s.set_inactive(!pscActive); });
    }
};

bool allAvailable(const TelemetryClient::Snapshot& snapshot)
{
    for (const auto& reading : snapshot.readings)
        if (!reading.connected || !reading.available) return false;
    return true;
}

int main()
{
    Fixture fixture;
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&fixture);
    auto server = builder.BuildAndStart();
    if (!server || port == 0) return 1;
    const std::string address = "127.0.0.1:" + std::to_string(port);
    int result = 0;
    try
    {
        TelemetryClient::Options options;
        options.staleAfter = 500ms;
        options.retryInitial = 50ms;
        options.retryMaximum = 100ms;
        TelemetryClient client(address, options);
        eventually([&] { return allAvailable(client.snapshot()); }, "Did not receive all streams");
        const auto first = client.snapshot();
        require(first.readings[TelemetryClient::Temperature].value == 23.5, "Wrong temperature");
        require(first.readings[TelemetryClient::Pressure].value == 101.2, "Wrong pressure");
        require(first.readings[TelemetryClient::Waveform].value == 1.65, "Wrong waveform");
        eventually([&] { return client.snapshot().readings[TelemetryClient::Waveform].sequence >= 600; },
                   "Waveform did not fill the bounded buffer");
        const auto full = client.snapshot();
        require(full.waveform.size() == TelemetryClient::waveformCapacity, "Waveform buffer not bounded");
        require(full.waveform.front().seconds < full.waveform.back().seconds, "Wrong waveform order");
        require(full.parameters[0].available && full.parameters[0].value == 5.2 && full.trends[0].size() == 1,
                "Heartbeat discarded or duplicated the slow parameter reading");
        fixture.pscActive = false;
        eventually([&] {
            auto state = client.snapshot();
            return !state.parameters[0].available && state.trends[0].empty() &&
                !state.readings[TelemetryClient::Waveform].available && state.waveform.empty();
        }, "PSC disconnection left stale waveform or trend");
        std::this_thread::sleep_for(600ms);
        require(client.snapshot().readings[TelemetryClient::Monitor].available,
                "Inactive heartbeat triggered watchdog");
        fixture.pscActive = true;
        eventually([&] { return client.snapshot().parameters[0].available && allAvailable(client.snapshot()); },
                   "PSC reconnection did not resume readings");

        fixture.dropWaveform = true;
        eventually([&] {
            const auto s = client.snapshot();
            return !s.readings[TelemetryClient::Waveform].available && s.waveform.empty();
        }, "Disconnected waveform remained visible");
        require(client.snapshot().readings[TelemetryClient::Temperature].available,
                "One disconnected stream interrupted another");
        fixture.dropWaveform = false;
        eventually([&] { return allAvailable(client.snapshot()); }, "Client did not reconnect");
        require(fixture.waveformCalls.load() >= 2, "No new waveform subscription");

        fixture.stall = true;
        eventually([&] {
            const auto s = client.snapshot();
            for (const auto& r : s.readings) if (r.available) return false;
            return true;
        }, "Silent server was not detected");
        fixture.stall = false;
        eventually([&] { return allAvailable(client.snapshot()); }, "Client did not recover after stall");
        auto before = std::chrono::steady_clock::now();
        client.stop();
        require(std::chrono::steady_clock::now() - before < 2s, "Stopping active reads blocked");
        server->Shutdown(std::chrono::system_clock::now() + 2s);
        server->Wait();

        TelemetryClient offline(address, options);
        std::this_thread::sleep_for(150ms);
        before = std::chrono::steady_clock::now();
        offline.stop();
        require(std::chrono::steady_clock::now() - before < 2s, "Stopping retry/connection blocked");
        std::cout << "PASS: streams, values, bounded buffer, independent reconnect, stale detection, shutdown\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        result = 1;
    }
    server->Shutdown(std::chrono::system_clock::now() + 2s);
    server->Wait();
    return result;
}
