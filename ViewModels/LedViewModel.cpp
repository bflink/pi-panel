#include "LedViewModel.h"
#include "../Controllers/LedController.h"

#include <exception>

LedViewModel::LedViewModel(
    LedController& ledController,
    QObject* parent)
    : QObject(parent),
      m_ledController(ledController)
{
}

bool LedViewModel::isLedOn() const
{
    return m_ledController.isOn();
}

void LedViewModel::toggle()
{
    try
    {
        m_ledController.toggle();
    }
    catch (const std::exception& error)
    {
        emit operationFailed(QString::fromUtf8(error.what()));
        return;
    }

    emit ledOnChanged();
}