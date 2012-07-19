// import QtQuick 1.0 // to target S60 5th Edition or Maemo 5
import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    id: driveGridRect
    color: "transparent"

    signal driveSelected (string driveName)

    property alias model: driveGrid.model
    property variant driveTypeTexts: ["No Drive", "Internal Drive", "Removable Drive", "Remote Drive", "Cdrom Drive",             "Internal Flash Drive", "Ram Drive"]
    property variant driveIcons:     ["",         "device.svg",     "memory_card.svg", "",             "music_player_update.svg", "memory_card.svg",      "device.svg"]
    property int currentIndex: driveGrid.currentIndex

    GridView {
        id: driveGrid
        anchors.fill: parent
        cellWidth: parent.width
        cellHeight: 80
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
            if (currentItem) currentItem.state = "normal";
        }
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
                    source: driveIcons[model.driveType]
                    anchors.verticalCenter: parent.verticalCenter
                }

                Column {
                    width: parent.width - driveIcon.width - 4
                    height: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        height: 40
                        color: (theme.inverted) ? "white" : "black"
                        font.pointSize: 18
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        text: model.logicalDrive + "  (" + driveTypeTexts[model.driveType] + ")"
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
                            width: (model.totalSpace - model.availableSpace) / model.totalSpace * parent.width
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
                            font.pointSize: 16
                            text: qsTr("Free") + ": " + Utility.formatFileSize(model.availableSpace, 1)
                                  + " / " + qsTr("Total") + ": " + Utility.formatFileSize(model.totalSpace, 1)
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
                    driveCellItem.state = "highlight";
                    driveGrid.currentIndex = index;
                }

                onClicked: {
                    // console.debug("driveCellItem clicked " + (parent.x + mouseX) + ", " + (parent.y + mouseY) );
                    driveGrid.currentItem.state = "normal";
                    driveGrid.currentDriveName = model.logicalDrive;
                    driveGridRect.driveSelected(model.logicalDrive);
                }
            }
        }
    }
}
