import QtQuick 2.4
import QtQuick.Controls 1.4 as QCtl
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2 as Wnd

import fhac.upns 1.0 as UPNS

StyledButton {
    id: root
    text: qsTr("Checkout")
    tooltip: qsTr("Open Dialog to choose checkout to work on")
    property var currentCheckoutName
    onClicked: chooseCheckoutDialog.visible = !chooseCheckoutDialog.visible
    Wnd.Window {
        id: chooseCheckoutDialog
        width: 420
        height: 260
        minimumHeight: height
        maximumHeight: height
        minimumWidth: width
        maximumWidth: width
        flags: Qt.Dialog
        title: qsTr("Choose Checkout")
        color: appStyle.backgroundColor
        ColumnLayout {
            anchors.fill: parent

            ListView {
                id: checkoutList
                delegate: RowLayout {
                        Image {
                            source: "image://icon/asset-green"
                        }
                        StyledLabel {
                            text: globalRepository.checkoutNames[index]
                            MouseArea {
                                anchors.fill: parent
                                onClicked: checkoutList.currentIndex = index
                            }
                        }
                    }

                model: globalRepository.checkoutNames
                highlight: Rectangle { color: appStyle.selectionColor }

                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            QCtl.Button {
                text: "+"
                onClicked: {
                    newCheckoutDialog.visible = !newCheckoutDialog.visible
                }
                Wnd.Window {
                    id: newCheckoutDialog
                    width: 420
                    height: 260
                    minimumHeight: height
                    maximumHeight: height
                    minimumWidth: width
                    maximumWidth: width
                    flags: Qt.Dialog
                    title: "Choose Checkout"
                    color: appStyle.backgroundColor
                    GridLayout {
                        StyledLabel {
                            text: "Branchname"
                            Layout.column: 0
                            Layout.row: 0
                        }
                        StyledTextField {
                            id: branchnameTextedit
                            text: "master"
                            Layout.column: 1
                            Layout.row: 0
                        }
                        StyledLabel {
                            text: "Checkoutname"
                            Layout.column: 0
                            Layout.row: 1
                        }
                        StyledTextField {
                            id: checkoutnameTextedit
                            Layout.column: 1
                            Layout.row: 1
                        }
                        StyledButton {
                            text: "Cancel"
                            onClicked: newCheckoutDialog.visible = false
                            Layout.column: 0
                            Layout.row: 2
                        }
                        StyledButton {
                            text: "Ok"
                            enabled: branchnameTextedit.text.trim().length !== 0
                                     && checkoutnameTextedit.text.trim().length !== 0
                            onClicked: {
                                globalRepository.createCheckout(branchnameTextedit.text, checkoutnameTextedit.text)
                                newCheckoutDialog.visible = false
                            }
                            Layout.column: 1
                            Layout.row: 2
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                StyledButton {
                    text: "Cancel"
                    onClicked: chooseCheckoutDialog.visible = false
                }
                StyledButton {
                    text: "Ok"
                    onClicked: {
                        root.currentCheckoutName = globalRepository.checkoutNames[checkoutList.currentIndex];
                        chooseCheckoutDialog.visible = false;
                    }
                }
            }
        }
    }
}
