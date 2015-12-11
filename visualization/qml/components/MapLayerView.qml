import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1

import "."

ListView {
    property var mapId
    id: mapLayers
    clip: true
    model: Globals.getMap(mapId).layers
    highlight: Rectangle {
        width: mapLayers.currentItem.width + 2
        height: mapLayers.currentItem.height
        color: palette.highlight
        radius: 1
        y: mapLayers.currentItem.y
    }
    highlightFollowsCurrentItem: false
    delegate: Text {
        renderType: Text.NativeRendering
        text: mapLayers.model[index].name.length===0?"<empty name>":mapLayers.model[index].name
        color: mapLayers.currentIndex == index?palette.highlightedText:palette.text
        MouseArea {
            anchors.fill: parent
            onClicked: mapLayers.currentIndex = index
        }
    }
    SystemPalette {
        id: palette
    }
}
