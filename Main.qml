import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 680
    height: 430
    visible: true
    title: "Pi Panel"

    required property QtObject ledViewModel

    Row {
        anchors.centerIn: parent
        spacing: 32

        Column {
            width: 300
            spacing: 16

            Rectangle {
                width: 72
                height: 72
                radius: 36
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
                color: "#555555"
                font.pixelSize: 13
                text: "ROOM TEMPERATURE"
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 34
                font.bold: true
                visible: window.ledViewModel.temperatureAvailable

                text: window.ledViewModel.temperatureFahrenheit.toFixed(1) + " °F"
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#666666"
                font.pixelSize: 13

                text: window.ledViewModel.analogAvailable
                      ? "Analog 1  " + window.ledViewModel.analogVoltage.toFixed(3) + " V"
                      : "Analog 1 unavailable"
            }

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: !window.ledViewModel.analogAvailable
                       || !window.ledViewModel.temperatureAvailable

                text: !window.ledViewModel.analogAvailable
                    ? window.ledViewModel.analogError
                    : window.ledViewModel.temperatureError
            }
        }

        Rectangle {
            width: 270
            height: 260
            anchors.verticalCenter: parent.verticalCenter
            radius: 6
            color: "#f3f5f5"
            border.color: "#d2d8da"

            Column {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Label {
                    color: "#4f5b60"
                    font.pixelSize: 16
                    font.bold: true
                    text: "System temperatures"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#d2d8da"
                }

                Repeater {
                    model: window.ledViewModel.systemTemperatures

                    Row {
                        width: 234
                        spacing: 8

                        Label {
                            width: 150
                            color: "#59666b"
                            elide: Text.ElideRight
                            font.pixelSize: 13
                            text: modelData.name
                        }

                        Label {
                            width: 76
                            horizontalAlignment: Text.AlignRight
                            color: "#263238"
                            font.pixelSize: 14
                            font.bold: true
                            text: Number(modelData.fahrenheit).toFixed(1) + " °F"
                        }
                    }
                }

                Label {
                    width: parent.width
                    color: "#778287"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    visible: window.ledViewModel.systemTemperatures.length === 0
                    text: "No system sensors available"
                }

                Item {
                    width: 1
                    height: 4
                }

                Label {
                    width: parent.width
                    color: "#778287"
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    text: "Internal component temperatures, not room temperature."
                }
            }
        }
    }
}