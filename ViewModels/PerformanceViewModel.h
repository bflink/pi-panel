#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <atomic>
#include <memory>

class QQuickWindow;
class TelemetryViewModel;

class PerformanceViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double cpuPercent READ cpuPercent NOTIFY updated)
    Q_PROPERTY(double memoryMiB READ memoryMiB NOTIFY updated)
    Q_PROPERTY(double temperatureCelsius READ temperatureCelsius NOTIFY updated)
    Q_PROPERTY(double framesPerSecond READ framesPerSecond NOTIFY updated)
    Q_PROPERTY(double samplesPerSecond READ samplesPerSecond NOTIFY updated)
public:
    explicit PerformanceViewModel(TelemetryViewModel& telemetry, QObject* parent = nullptr);
    void attachWindow(QQuickWindow* window);
    double cpuPercent() const { return cpu_; }
    double memoryMiB() const { return memory_; }
    double temperatureCelsius() const { return temperature_; }
    double framesPerSecond() const { return fps_; }
    double samplesPerSecond() const { return samples_; }
signals:
    void updated();
private:
    void refresh();
    TelemetryViewModel& telemetry_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    std::shared_ptr<std::atomic<unsigned>> frames_ = std::make_shared<std::atomic<unsigned>>(0);
    QMetaObject::Connection frameConnection_;
    double previousCpu_ = -1;
    quint64 previousSamples_ = 0;
    double cpu_ = -1, memory_ = -1, temperature_ = -1, fps_ = 0, samples_ = 0;
    QString temperaturePath_;
};
