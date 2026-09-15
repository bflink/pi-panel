#include "ViewModels/PerformanceViewModel.h"
#include "ViewModels/TelemetryViewModel.h"
#include <QDir>
#include <QFile>
#include <QQuickWindow>
#include <algorithm>
#include <time.h>

namespace {
double cpuSeconds()
{
#ifdef Q_OS_LINUX
    timespec time{};
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time) == 0)
        return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_nsec) / 1e9;
#endif
    return -1;
}
double residentMiB()
{
    QFile file(QStringLiteral("/proc/self/status"));
    if (!file.open(QIODevice::ReadOnly)) return -1;
    // procfs files report size zero: readAll instead of relying on file size.
    for (const auto& line : file.readAll().split('\n')) {
        if (!line.startsWith("VmRSS:")) continue;
        const auto fields = line.simplified().split(' ');
        bool ok = false;
        const double kib = fields.size() > 1 ? fields[1].toDouble(&ok) : -1;
        return ok ? kib / 1024 : -1;
    }
    return -1;
}
QString cpuTemperaturePath()
{
    QDir thermal(QStringLiteral("/sys/class/thermal"));
    for (const auto& zone : thermal.entryList({QStringLiteral("thermal_zone*")}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        const auto path = thermal.filePath(zone);
        QFile type(path + QStringLiteral("/type"));
        if (!type.open(QIODevice::ReadOnly)) continue;
        const auto name = type.readAll().trimmed().toLower();
        if (name == "cpu-thermal" || name == "cpu_thermal" || name == "bcm2835_thermal")
            return path + QStringLiteral("/temp");
    }
    return {};
}
}

PerformanceViewModel::PerformanceViewModel(TelemetryViewModel& telemetry, QObject* parent)
    : QObject(parent), telemetry_(telemetry), temperaturePath_(cpuTemperaturePath())
{
    previousCpu_ = cpuSeconds();
    previousSamples_ = telemetry_.receivedSamples();
    elapsed_.start();
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &PerformanceViewModel::refresh);
    timer_.start();
}

void PerformanceViewModel::attachWindow(QQuickWindow* window)
{
    QObject::disconnect(frameConnection_);
    if (!window) return;
    // frameSwapped may run on the render thread. Only touch a shared atomic;
    // its lifetime is independent of this model during window destruction.
    frameConnection_ = connect(window, &QQuickWindow::frameSwapped, window,
        [counter = frames_] { counter->fetch_add(1, std::memory_order_relaxed); }, Qt::DirectConnection);
}

void PerformanceViewModel::refresh()
{
    const double seconds = static_cast<double>(elapsed_.nsecsElapsed()) / 1e9;
    elapsed_.restart();
    if (seconds <= 0) return;
    const double nowCpu = cpuSeconds();
    cpu_ = nowCpu >= 0 && previousCpu_ >= 0 ? std::max(0.0, 100 * (nowCpu - previousCpu_) / seconds) : -1;
    previousCpu_ = nowCpu;
    memory_ = residentMiB();
    temperature_ = -1;
    if (!temperaturePath_.isEmpty()) {
        QFile temperatureFile(temperaturePath_);
        if (temperatureFile.open(QIODevice::ReadOnly)) {
            bool ok = false;
            const double value = temperatureFile.readAll().trimmed().toDouble(&ok);
            if (ok && value >= 0 && value < 200000) temperature_ = value / 1000;
        }
    }
    fps_ = static_cast<double>(frames_->exchange(0, std::memory_order_relaxed)) / seconds;
    const auto total = telemetry_.receivedSamples();
    samples_ = total >= previousSamples_ ? static_cast<double>(total - previousSamples_) / seconds : 0;
    previousSamples_ = total;
    emit updated();
}
