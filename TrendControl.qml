import QtQuick
import QtQuick.Controls

Column {
    id: trend
    required property QtObject telemetryViewModel
    property int initialSelection: 1
    readonly property var channel: selector.currentIndex > 0
        ? telemetryViewModel.trendChannels[selector.currentIndex - 1] : null
    spacing: 6
    Row {
        width: parent.width
        spacing: 8
        ComboBox {
            id: selector
            objectName: "trendSelector"
            width: Math.min(160, parent.width * 0.4)
            model: ["Off", "CO", "SV", "SVV", "HR", "STO2 A1", "STO2 A2", "STO2 B1", "STO2 B2"]
            currentIndex: trend.initialSelection
        }
        Label {
            width: parent.width - selector.width - parent.spacing
            anchors.verticalCenter: parent.verticalCenter
            font.bold: true
            text: trend.channel ? (trend.channel.available
                ? Number(trend.channel.value).toFixed(trend.channel.name === "CO" ? 2 : 1) + " " + trend.channel.unit
                : "— " + trend.channel.unit) : "Trend off"
        }
    }
    Canvas {
        id: plot
        width: parent.width
        height: 115
        visible: trend.channel !== null
        onWidthChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.fillStyle = "#142332"
            ctx.fillRect(0, 0, width, height)
            if (!trend.channel) return
            const left = 30, right = width - 8, top = 10, bottom = height - 18
            const upper = trend.channel.upper
            ctx.font = "10px sans-serif"
            ctx.fillStyle = "#c6d8e5"
            ctx.fillText(String(upper), 2, top + 5)
            ctx.fillText("0", 2, bottom)
            ctx.fillText("−10 min", left, height - 3)
            ctx.fillText("Latest", right - 32, height - 3)
            ctx.strokeStyle = "#314657"
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(left, top); ctx.lineTo(left, bottom); ctx.lineTo(right, bottom)
            ctx.stroke()
            const points = trend.channel.points
            if (!trend.channel.available || points.length === 0) return
            const end = points[points.length - 1].seconds
            ctx.strokeStyle = "#f2c66d"
            ctx.fillStyle = "#f2c66d"
            ctx.lineWidth = 2
            ctx.beginPath()
            let lastX = 0, lastY = 0
            for (let i = 0; i < points.length; ++i) {
                const x = left + (right - left) * (points[i].seconds - end + 600) / 600
                const y = bottom - (bottom - top) * Math.max(0, Math.min(upper, points[i].value)) / upper
                if (i === 0 || points[i].seconds - points[i - 1].seconds > trend.channel.interval * 1.5)
                    ctx.moveTo(x, y)
                else ctx.lineTo(x, y)
                lastX = x; lastY = y
            }
            ctx.stroke()
            ctx.beginPath(); ctx.arc(lastX, lastY, 3, 0, Math.PI * 2); ctx.fill()
        }
    }
    Label {
        visible: trend.channel !== null
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: 11
        text: trend.channel ? trend.channel.status + " · updates every " + trend.channel.interval + " s" : ""
    }
    onChannelChanged: plot.requestPaint()
}
