import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Rectangle {
    id: driveGridRect
    color: "transparent"

    signal driveSelected (string driveName, int index)

    property alias model: driveGrid.model
    property variant driveTypeTexts: [appInfo.emptyStr+qsTr("No Drive"), appInfo.emptyStr+qsTr("Internal Drive"), appInfo.emptyStr+qsTr("Removable Drive"), appInfo.emptyStr+qsTr("Remote Drive"), appInfo.emptyStr+qsTr("Cdrom Drive"), appInfo.emptyStr+qsTr("Internal Flash Drive"), appInfo.emptyStr+qsTr("Ram Drive"), appInfo.emptyStr+qsTr("Cloud Drive")]
    property variant driveIcons:     ["", "device.svg", "memory_card.svg", "", "music_player_update.svg", "memory_card.svg", "device.svg", ""]
    property int currentIndex: driveGrid.currentIndex
    property bool busy: false

    GridView {
        id: driveGrid
        anchors.fill: parent
        cellWidth: parent.width
        cellHeight: 70
        delegate: driveCell
        highlight: Rectangle {
            gradient: highlightGradient
        }
        highlightFollowsCurrentItem: false
        highlightMoveDuration: 0
        focus: true
        currentIndex: -1

        property string currentDriveName: ""

        onMovementStarted: {
            if (currentItem) {
                busy = false;
                currentItem.state = "normal";
            }
        }
    }

    ScrollDecorator {
        id: scrollbar
        flickableItem: driveGrid
    }

    Gradient {
        id: highlightGradient
        GradientStop {
            position: 0
            color: "#0080D0"
        }

        GradientStop {
            position: 1
            color: "#53A3E6"
        }
    }

    Component {
        id: driveCell

        Rectangle {
            id: driveCellItem
            width: driveGrid.cellWidth
            height: driveGrid.cellHeight
            color: "transparent"

            state: "normal"
            states: [
                State {
                    name: "highlight"
                    PropertyChanges {
                        target: driveCellItem
                        gradient: highlightGradient
                    }
                },
                State {
                    name: "normal"
                }
            ]

            Row {
                anchors.fill: parent
                spacing: 2
                Image {
                    id: driveIcon
                    width: 67
                    height: 57
                    source: (iconSource != "") ? iconSource : driveIcons[model.driveType]
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode: Image.PreserveAspectFit
                }

                Column {
                    width: parent.width - driveIcon.width - 4
                    height: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        height: 35
                        color: (!window.platformInverted) ? "white" : "black"
                        font.pointSize: 6
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        text: ((model.name != "") ? model.name : model.logicalDrive) + "  (" + driveTypeTexts[model.driveType] + ")"
                    }

                    Rectangle {
                        width: parent.width
                        height: 30
                        gradient: Gradient {
                            GradientStop {
                                position: 0
                                color: "#141414"
                            }

                            GradientStop {
                                position: 0.510
                                color: "#4d4d4d"
                            }

                            GradientStop {
                                position: 1
                                color: "#141414"
                            }
                        }

                        Rectangle {
                            width: (model.totalSpace < 0) ? 0 : ((model.totalSpace - model.availableSpace) / model.totalSpace * parent.width)
                            height: parent.height
                            gradient: Gradient {
                                GradientStop {
                                    position: 0
                                    color: "#0091d9"
                                }

                                GradientStop {
                                    position: 0.500
                                    color: "#00aaff"
                                }

                                GradientStop {
                                    position: 1
                                    color: "#001f3c"
                                }
                            }
                        }
                        Text {
                            width: parent.width
                            height: parent.height
                            color: "white"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            style: Text.Outline
                            font.pointSize: 6
                            text: appInfo.emptyStr+(model.totalSpace <= 0 ? qsTr("Not available") : (qsTr("Free") + ": " + Utility.formatFileSize(model.availableSpace, 1) + " / " + appInfo.emptyStr+qsTr("Total") + ": " + Utility.formatFileSize(model.totalSpace, 1)))
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#202020"
                anchors.bottom: parent.bottom
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton

                onPressed: {
                    if (!busy) {
                        busy = true;
                        driveCellItem.state = "highlight";
                        driveGrid.currentIndex = index;
                    } else {
                        mouse.accepted = false;
                    }
                }

                onClicked: {
                    // console.debug("driveCellItem clicked " + (parent.x + mouseX) + ", " + (parent.y + mouseY) );
                    driveGrid.currentItem.state = "normal";
                    driveGrid.currentDriveName = model.logicalDrive;
                    driveGridRect.driveSelected(model.logicalDrive, index);
                }
            }
        }
    }
}
