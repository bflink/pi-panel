import QtQuick
import QtQuick.Controls

ScrollView {
    id: panel
    required property QtObject telemetryViewModel
    padding: 16
    contentWidth: availableWidth

    Column {
        width: panel.availableWidth
        spacing: 12
        Label { text: "Simulated data from Windows"; font.bold: true; font.pixelSize: 20 }
        Label { text: "Windows PC address and port" }
        Row {
            width: parent.width
            spacing: 8
            TextField {
                id: address
                width: parent.width - connectButton.width - parent.spacing
                placeholderText: "192.168.1.50:50051"
                text: panel.telemetryViewModel.endpoint
                enabled: !panel.telemetryViewModel.running
                selectByMouse: true
                onAccepted: if (text.trim().length > 0) panel.telemetryViewModel.connectToServer(text)
            }
            Button {
                id: connectButton
                text: panel.telemetryViewModel.running ? "Disconnect" : "Connect"
                enabled: panel.telemetryViewModel.running || address.text.trim().length > 0
                onClicked: {
                    if (panel.telemetryViewModel.running) panel.telemetryViewModel.disconnectFromServer()
                    else panel.telemetryViewModel.connectToServer(address.text)
                }
            }
        }
        Row {
            width: parent.width
            spacing: 8
            Column {
                width: (parent.width - 2 * parent.spacing) / 3
                spacing: 4
                Label { text: "Temperature · 1 Hz"; font.pixelSize: 12 }
                Label {
                    font.pixelSize: 20
                    font.bold: true
                    text: panel.telemetryViewModel.temperatureAvailable
                        ? panel.telemetryViewModel.temperatureCelsius.toFixed(2) + " °C" : "—"
                }
                Label {
                    width: parent.width; wrapMode: Text.Wrap; font.pixelSize: 11
                    text: panel.telemetryViewModel.temperatureStatus
                }
            }
            Column {
                width: (parent.width - 2 * parent.spacing) / 3
                spacing: 4
                Label { text: "Pressure · 10 Hz"; font.pixelSize: 12 }
                Label {
                    font.pixelSize: 20
                    font.bold: true
                    text: panel.telemetryViewModel.pressureAvailable
                        ? panel.telemetryViewModel.pressureKilopascals.toFixed(2) + " kPa" : "—"
                }
                Label {
                    width: parent.width; wrapMode: Text.Wrap; font.pixelSize: 11
                    text: panel.telemetryViewModel.pressureStatus
                }
            }
            Column {
                width: (parent.width - 2 * parent.spacing) / 3
                spacing: 4
                Label { text: "Waveform · 100 Hz"; font.pixelSize: 12 }
                Label {
                    font.pixelSize: 20
                    font.bold: true
                    text: panel.telemetryViewModel.waveformAvailable
                        ? panel.telemetryViewModel.waveformVolts.toFixed(3) + " V" : "—"
                }
                Label {
                    width: parent.width; wrapMode: Text.Wrap; font.pixelSize: 11
                    text: panel.telemetryViewModel.waveformStatus
                }
            }
        }
        Canvas {
            id: chart
            width: parent.width
            height: 150
            onWidthChanged: requestPaint()
            onVisibleChanged: if (visible) requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.fillStyle = "#142332"
                ctx.fillRect(0, 0, width, height)
                ctx.strokeStyle = "#314657"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(0, height / 2)
                ctx.lineTo(width, height / 2)
                ctx.stroke()
                const points = panel.telemetryViewModel.waveformPoints
                if (points.length < 2) return
                const end = points[points.length - 1].seconds
                ctx.strokeStyle = "#5de3c4"
                ctx.lineWidth = 2
                ctx.beginPath()
                let first = true
                for (let i = 0; i < points.length; ++i) {
                    const x = width * (points[i].seconds - end + 5) / 5
                    if (x < 0) continue
                    const y = height * (1 - Math.max(0, Math.min(3.3, points[i].volts)) / 3.3)
                    if (first) { ctx.moveTo(x, y); first = false }
                    else ctx.lineTo(x, y)
                }
                ctx.stroke()
            }
            Connections {
                target: panel.telemetryViewModel
                function onUpdated() { if (chart.visible) chart.requestPaint() }
            }
        }
        Label { text: "Last 5 seconds · 0–3.3 V" }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: "Use the PC’s LAN address. The server must be running with LAN connections enabled."
        }
    }
}
