#pragma once

#include <QObject>
#include <QString>

class LedController;

class LedViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool ledOn READ isLedOn NOTIFY ledOnChanged)

public:
    explicit LedViewModel(
        LedController& ledController,
        QObject* parent = nullptr);

    bool isLedOn() const;

    Q_INVOKABLE void toggle();

signals:
    void ledOnChanged();
    void operationFailed(const QString& message);

private:
    LedController& m_ledController;
};