import QtQuick 2.15
import QtQuick.Window 2.15
import ImageDiffer 1.0
import QtQuick.Controls 2.15

Window {
    id : win
    width: 1024
    height: 1024
    visible: true
    title: qsTr("Image Differ")

    ImageDiffer {
        id: differ
        gain: slider.value
    }

    Component.onCompleted: {
        differ.diff("/<filepath 1>",
                    "/<filepath 2>")
    }

    ListView {
        anchors.fill: parent
        model: differ.imageModel
        delegate: Image {
            source: modelData
            width: win.width
            cache: false
            fillMode: Image.PreserveAspectFit
            smooth: false
        }
    }

    Slider {
        id: slider
        from: 1.0
        to: 100.0
        value: 1.0
        orientation: Qt.Horizontal
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: 20
            rightMargin: 20
            bottomMargin: 40
        }
    }
}
