import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 420
    height: 300
    visible: true
    title: "Pi Panel"

    property bool indicatorOn: false

    Column {
        anchors.centerIn: parent
        spacing: 20

        Rectangle {
            width: 80
            height: 80
            radius: 40
            anchors.horizontalCenter: parent.horizontalCenter

            color: window.indicatorOn ? "limegreen" : "#444444"
            border.color: "#222222"
            border.width: 2
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: window.indicatorOn ? "Indicator ON" : "Indicator OFF"
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Toggle indicator"

            onClicked: {
                window.indicatorOn = !window.indicatorOn
            }
        }
    }
}