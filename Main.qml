import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 420
    height: 300
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
    }
}