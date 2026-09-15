#include "ViewModels/TelemetryViewModel.h"
#include <QVariantMap>
#include <exception>

TelemetryViewModel::TelemetryViewModel(QObject* parent) : QObject(parent)
{
    refreshTimer_.setInterval(50);
    connect(&refreshTimer_, &QTimer::timeout, this, &TelemetryViewModel::refresh);
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
    client_.reset();
    snapshot_ = {};
    waveformPoints_.clear();
    idleStatus_ = QStringLiteral("Not connected");
    emit updated();
}

void TelemetryViewModel::refresh()
{
    if (!client_) return;
    auto next = client_->snapshot();
    if (next.revision == snapshot_.revision) return;
    snapshot_ = std::move(next);
    waveformPoints_.clear();
    waveformPoints_.reserve(static_cast<qsizetype>(snapshot_.waveform.size()));
    for (const auto& point : snapshot_.waveform)
        waveformPoints_.append(QVariantMap{{QStringLiteral("seconds"), point.seconds},
                                           {QStringLiteral("volts"), point.volts}});
    // Only the GUI thread changes QObject properties; workers never queue one
    // Qt event per network sample. Rendering remains bounded at 20 Hz.
    emit updated();
}
