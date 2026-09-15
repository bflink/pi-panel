#include "ViewModels/TelemetryViewModel.h"
#include <QVariantMap>
#include <exception>

TelemetryViewModel::TelemetryViewModel(QObject* parent) : QObject(parent)
{
    refreshTimer_.setInterval(50);
    connect(&refreshTimer_, &QTimer::timeout, this, &TelemetryViewModel::refresh);
    refreshTrends();
}

TelemetryViewModel::~TelemetryViewModel()
{
    refreshTimer_.stop();
    client_.reset();
}

QString TelemetryViewModel::status(TelemetryClient::Stream stream) const
{
    return client_ ? QString::fromStdString(reading(stream).status) : idleStatus_;
}

void TelemetryViewModel::connectToServer(const QString& address)
{
    disconnectFromServer();
    endpoint_ = address.trimmed();
    emit endpointChanged();
    if (endpoint_.isEmpty())
    {
        idleStatus_ = QStringLiteral("Enter the Windows PC address and port");
        emit updated();
        return;
    }
    try
    {
        client_ = std::make_unique<TelemetryClient>(endpoint_.toStdString());
        refresh();
        refreshTimer_.start();
    }
    catch (const std::exception& e)
    {
        idleStatus_ = QString::fromUtf8(e.what());
    }
    emit updated();
}

void TelemetryViewModel::disconnectFromServer()
{
    refreshTimer_.stop();
    if (client_) {
        client_->stop();
        completedSamples_ += client_->receivedSamples();
    }
    client_.reset();
    snapshot_ = {};
    waveformPoints_.clear();
    refreshTrends();
    idleStatus_ = QStringLiteral("Not connected");
    emit updated();
}

void TelemetryViewModel::refresh()
{
    if (!client_) return;
    auto next = client_->snapshot();
    if (next.revision == snapshot_.revision) return;
    bool trendsChanged = false;
    for (std::size_t i = 0; i < next.parameters.size(); ++i)
        trendsChanged = trendsChanged || next.parameters[i].sequence != snapshot_.parameters[i].sequence ||
            next.parameters[i].status != snapshot_.parameters[i].status ||
            next.parameters[i].available != snapshot_.parameters[i].available;
    snapshot_ = std::move(next);
    if (trendsChanged) refreshTrends();
    waveformPoints_.clear();
    waveformPoints_.reserve(static_cast<qsizetype>(snapshot_.waveform.size()));
    for (const auto& point : snapshot_.waveform)
        waveformPoints_.append(QVariantMap{{QStringLiteral("seconds"), point.seconds},
                                           {QStringLiteral("volts"), point.volts}});
    // Only the GUI thread changes QObject properties; workers never queue one
    // Qt event per network sample. Rendering remains bounded at 20 Hz.
    emit updated();
}

void TelemetryViewModel::refreshTrends()
{
    static const char* names[] = {"CO", "SV", "SVV", "HR", "STO2 A1", "STO2 A2", "STO2 B1", "STO2 B2"};
    static const char* units[] = {"L/min", "mL", "%", "beats/min", "%", "%", "%", "%"};
    static const double upper[] = {10, 120, 30, 140, 100, 100, 100, 100};
    trendChannels_.clear();
    for (std::size_t i = 0; i < snapshot_.parameters.size(); ++i) {
        const auto& reading = snapshot_.parameters[i];
        QVariantList points;
        for (const auto& point : snapshot_.trends[i])
            points.append(QVariantMap{{"seconds", point.seconds}, {"value", point.volts}});
        trendChannels_.append(QVariantMap{
            {"name", QString::fromLatin1(names[i])}, {"unit", QString::fromLatin1(units[i])},
            {"upper", upper[i]}, {"value", reading.value}, {"available", reading.available},
            {"status", client_ ? QString::fromStdString(reading.status) : QStringLiteral("Not connected")},
            {"points", points}, {"interval", i < 4 ? 20 : 2}});
    }
    emit trendsChanged();
}
