import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 420
    height: 380
    visible: true
    title: "Pi Panel"

    required property QtObject ledViewModel

    Column {
        anchors.centerIn: parent
        spacing: 20

        Rectangle {
            width: 80
            height: 80
            radius: 40
            anchors.horizontalCenter: parent.horizontalCenter

            color: window.ledViewModel.ledOn
                   ? "limegreen"
                   : "#444444"

            border.color: "#222222"
            border.width: 2
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter

            text: window.ledViewModel.ledOn
                  ? "LED ON"
                  : "LED OFF"
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Toggle LED"

            onClicked: window.ledViewModel.toggle()
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 22
            font.bold: true

            text: window.ledViewModel.analogAvailable
                  ? "Analog 1: " + window.ledViewModel.analogVoltage.toFixed(3) + " V"
                  : "Analog 1 unavailable"
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 28
            font.bold: true
            visible: window.ledViewModel.temperatureAvailable

            text: window.ledViewModel.temperatureFahrenheit.toFixed(1) + " °F"
        }

        Label {
            width: Math.min(360, window.width - 40)
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            visible: !window.ledViewModel.analogAvailable
                   || !window.ledViewModel.temperatureAvailable

            text: !window.ledViewModel.analogAvailable
                ? window.ledViewModel.analogError
                : window.ledViewModel.temperatureError
        }
    }
}