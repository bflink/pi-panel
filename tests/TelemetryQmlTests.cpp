#include "ViewModels/TelemetryViewModel.h"
#include <grpcpp/grpcpp.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QVariant>
#include <cmath>
#include <iostream>
#include <thread>

// A QObject stand-in lets the real two-tab window load without GPIO hardware.
class LedMock final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ledOn MEMBER ledOn CONSTANT)
    Q_PROPERTY(bool analogAvailable MEMBER analogAvailable CONSTANT)
    Q_PROPERTY(bool temperatureAvailable MEMBER temperatureAvailable CONSTANT)
    Q_PROPERTY(double analogVoltage MEMBER analogVoltage CONSTANT)
    Q_PROPERTY(double temperatureFahrenheit MEMBER temperatureFahrenheit CONSTANT)
    Q_PROPERTY(QString analogError MEMBER error CONSTANT)
    Q_PROPERTY(QString temperatureError MEMBER error CONSTANT)
public:
    bool ledOn = false, analogAvailable = true, temperatureAvailable = true;
    double analogVoltage = 1.65, temperatureFahrenheit = 72;
    QString error;
    Q_INVOKABLE void toggle() {}
};

namespace wire = pi::telemetry::v1;
class DisplayFixture final : public wire::Telemetry::Service
{
    template<class Sample, class Set>
    grpc::Status send(grpc::ServerContext* c, grpc::ServerWriter<Sample>* w, Set set)
    {
        for (std::uint64_t i = 0; !c->IsCancelled(); ++i)
        {
            Sample s;
            s.mutable_metadata()->set_sequence(i);
            s.mutable_metadata()->set_elapsed_seconds(static_cast<double>(i) / 100.0);
            set(s, static_cast<double>(i) / 100.0);
            if (!w->Write(s)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return grpc::Status::OK;
    }
public:
    grpc::Status StreamTemperature(grpc::ServerContext* c, const wire::StreamRequest*,
                                   grpc::ServerWriter<wire::TemperatureSample>* w) override
    { return send(c, w, [](auto& s, double) { s.set_celsius(23.5); }); }
    grpc::Status StreamPressure(grpc::ServerContext* c, const wire::StreamRequest*,
                                grpc::ServerWriter<wire::PressureSample>* w) override
    { return send(c, w, [](auto& s, double) { s.set_kilopascals(101.2); }); }
    grpc::Status StreamWaveform(grpc::ServerContext* c, const wire::StreamRequest*,
                                grpc::ServerWriter<wire::WaveformSample>* w) override
    { return send(c, w, [](auto& s, double t) { s.set_volts(1.65 + 1.2 * std::sin(12.5663706 * t)); }); }
};

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    DisplayFixture service;
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    if (!server || !port) return 1;
    int result = 1;
    {
        LedMock led;
        TelemetryViewModel telemetry;
        QQmlApplicationEngine engine;
        bool qmlWarning = false;
        QObject::connect(&engine, &QQmlEngine::warnings, &app,
                         [&](const QList<QQmlError>&) { qmlWarning = true; });
        engine.setInitialProperties({{QStringLiteral("ledViewModel"), QVariant::fromValue(&led)},
            {QStringLiteral("telemetryViewModel"), QVariant::fromValue(&telemetry)}});
        engine.load(QUrl::fromLocalFile(QStringLiteral(PI_PANEL_SOURCE_DIR "/Main.qml")));
        if (!engine.rootObjects().isEmpty())
        {
            auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            auto* tabs = engine.rootObjects().first()->findChild<QObject*>(QStringLiteral("telemetryTabs"));
            if (window && tabs)
            {
                tabs->setProperty("currentIndex", 1);
                telemetry.connectToServer(QStringLiteral("127.0.0.1:%1").arg(port));
                QTimer::singleShot(1500, &app, [&] {
                    const bool ready = telemetry.temperatureAvailable() && telemetry.pressureAvailable() &&
                        telemetry.waveformAvailable() && telemetry.waveformPoints().size() > 5;
                    const bool captured = argc <= 1 ||
                        window->grabWindow().save(QString::fromLocal8Bit(argv[1]));
                    telemetry.disconnectFromServer();
                    const bool cleared = !telemetry.running() && !telemetry.temperatureAvailable() &&
                        telemetry.waveformPoints().isEmpty();
                    result = ready && cleared && !qmlWarning && captured ? 0 : 1;
                    app.quit();
                });
                app.exec();
            }
        }
    }
    server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(2));
    server->Wait();
    if (!result) std::cout << "PASS: QML window, live Qt properties, waveform, disconnect\n";
    return result;
}

#include "TelemetryQmlTests.moc"
