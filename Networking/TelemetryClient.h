#pragma once

#include "telemetry.grpc.pb.h"
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// No Qt or GPIO dependency: workers only update a bounded, synchronized snapshot.
class TelemetryClient final
{
public:
    enum Stream : std::size_t { Temperature, Pressure, Waveform, StreamCount };
    struct Reading
    {
        bool connected = false;
        bool available = false;
        double value = 0;
        std::uint64_t sequence = 0;
        std::string status = "Connecting";
    };
    struct Point { double seconds; double volts; };
    struct Snapshot
    {
        std::array<Reading, StreamCount> readings;
        std::vector<Point> waveform;
        std::uint64_t revision = 0;
    };
    struct Options
    {
        std::chrono::milliseconds staleAfter{5000};
        std::chrono::milliseconds retryInitial{500};
        std::chrono::milliseconds retryMaximum{8000};
    };
    static constexpr std::size_t waveformCapacity = 500;

    explicit TelemetryClient(const std::string& address);
    TelemetryClient(const std::string& address, Options options);
    ~TelemetryClient();
    TelemetryClient(const TelemetryClient&) = delete;
    TelemetryClient& operator=(const TelemetryClient&) = delete;
    Snapshot snapshot() const;
    // Call from the owning thread. Cancels reads, wakes retries, then joins workers.
    void stop();

private:
    template<class Sample, class Start, class Value>
    void subscribe(Stream stream, Start start, Value value);
    void watchForStalls();
    bool record(Stream stream, double value, const pi::telemetry::v1::SampleMetadata& metadata);

    Options options_;
    std::unique_ptr<pi::telemetry::v1::Telemetry::Stub> stub_;
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    bool stopping_ = false;
    std::array<grpc::ClientContext*, StreamCount> contexts_{};
    std::array<std::chrono::steady_clock::time_point, StreamCount> lastActivity_{};
    std::array<Reading, StreamCount> readings_;
    std::deque<Point> waveform_;
    std::uint64_t revision_ = 0;
    std::array<std::thread, StreamCount> workers_;
    std::thread watchdog_;
};
