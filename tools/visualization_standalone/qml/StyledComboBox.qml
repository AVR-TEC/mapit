import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

ComboBox {
    style: ComboBoxStyle {
        background: Rectangle {
            implicitHeight: appStyle.controlHeight
            implicitWidth: appStyle.controlHeight*4
            color: appStyle.itemBackgroundColor
        }
        textColor: appStyle.textColor
        selectionColor: appStyle.selectionColor
        selectedTextColor: appStyle.selectionBorderColor
    }
}