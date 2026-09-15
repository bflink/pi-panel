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
    grpc::Status StreamMonitor(grpc::ServerContext* c, const wire::MonitorRequest*,
                               grpc::ServerWriter<wire::MonitorFrame>* w) override
    {
        std::uint64_t sequence = 0;
        while (!c->IsCancelled()) {
            wire::MonitorFrame f;
            f.mutable_metadata()->set_sequence(sequence);
            f.mutable_metadata()->set_elapsed_seconds(static_cast<double>(sequence) * 2);
            f.set_psc_cable_connected(true); f.set_psc_sensor_connected(true);
            const double values[] = {5.1, 70, 9, 73, 70, 72, 74, 76};
            for (int i = 0; i < 8; ++i) {
                const auto type = static_cast<wire::ParameterType>(i + 1);
                if (i >= 4) f.add_connected_sto2(type);
                auto* v = f.add_values(); v->set_type(type);
                v->set_value(values[i] + std::sin(static_cast<double>(sequence) / 5));
            }
            ++sequence;
            if (!w->Write(f)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return grpc::Status::OK;
    }
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
                window->resize(480, 960);
                telemetry.connectToServer(QStringLiteral("127.0.0.1:%1").arg(port));
                // Let the five-second sweep wrap before checking the window.
                QTimer::singleShot(6500, &app, [&] {
                    const auto selectors = window->findChildren<QObject*>(QStringLiteral("trendSelector"));
                    bool selectionWorks = selectors.size() == 2;
                    if (selectionWorks) {
                        selectors[0]->setProperty("currentIndex", 0);
                        selectionWorks = selectors[0]->property("currentIndex").toInt() == 0;
                        selectors[0]->setProperty("currentIndex", 1);
                        selectors[1]->setProperty("currentIndex", 8);
                        selectionWorks = selectionWorks && selectors[1]->property("currentIndex").toInt() == 8;
                    }
                    const bool ready = telemetry.temperatureAvailable() && telemetry.pressureAvailable() &&
                        telemetry.waveformAvailable() && telemetry.waveformPoints().size() > 5 &&
                        telemetry.trendChannels().size() == 8 &&
                        telemetry.trendChannels()[0].toMap()["points"].toList().size() > 5 && selectionWorks;
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
