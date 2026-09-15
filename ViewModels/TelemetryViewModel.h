#pragma once
#include "Networking/TelemetryClient.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <memory>

class TelemetryViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString endpoint READ endpoint NOTIFY endpointChanged)
    Q_PROPERTY(bool running READ running NOTIFY updated)
    Q_PROPERTY(QString temperatureStatus READ temperatureStatus NOTIFY updated)
    Q_PROPERTY(QString pressureStatus READ pressureStatus NOTIFY updated)
    Q_PROPERTY(QString waveformStatus READ waveformStatus NOTIFY updated)
    Q_PROPERTY(bool temperatureAvailable READ temperatureAvailable NOTIFY updated)
    Q_PROPERTY(bool pressureAvailable READ pressureAvailable NOTIFY updated)
    Q_PROPERTY(bool waveformAvailable READ waveformAvailable NOTIFY updated)
    Q_PROPERTY(double temperatureCelsius READ temperatureCelsius NOTIFY updated)
    Q_PROPERTY(double pressureKilopascals READ pressureKilopascals NOTIFY updated)
    Q_PROPERTY(double waveformVolts READ waveformVolts NOTIFY updated)
    Q_PROPERTY(QVariantList waveformPoints READ waveformPoints NOTIFY updated)
public:
    explicit TelemetryViewModel(QObject* parent = nullptr);
    ~TelemetryViewModel() override;
    QString endpoint() const { return endpoint_; }
    bool running() const { return client_ != nullptr; }
    QString temperatureStatus() const { return status(TelemetryClient::Temperature); }
    QString pressureStatus() const { return status(TelemetryClient::Pressure); }
    QString waveformStatus() const { return status(TelemetryClient::Waveform); }
    bool temperatureAvailable() const { return reading(TelemetryClient::Temperature).available; }
    bool pressureAvailable() const { return reading(TelemetryClient::Pressure).available; }
    bool waveformAvailable() const { return reading(TelemetryClient::Waveform).available; }
    double temperatureCelsius() const { return reading(TelemetryClient::Temperature).value; }
    double pressureKilopascals() const { return reading(TelemetryClient::Pressure).value; }
    double waveformVolts() const { return reading(TelemetryClient::Waveform).value; }
    QVariantList waveformPoints() const { return waveformPoints_; }
    Q_INVOKABLE void connectToServer(const QString& address);
    Q_INVOKABLE void disconnectFromServer();
signals:
    void endpointChanged();
    void updated();
private:
    void refresh();
    const TelemetryClient::Reading& reading(TelemetryClient::Stream stream) const
    { return snapshot_.readings[stream]; }
    QString status(TelemetryClient::Stream stream) const;
    QString endpoint_;
    QString idleStatus_ = QStringLiteral("Not connected");
    std::unique_ptr<TelemetryClient> client_;
    TelemetryClient::Snapshot snapshot_;
    QVariantList waveformPoints_;
    QTimer refreshTimer_;
};
