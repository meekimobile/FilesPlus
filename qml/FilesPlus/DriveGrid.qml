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
        cellHeight: 64
        delegate: driveCell
        highlight: Rectangle {
            border.color: "blue"
            border.width: 1
            color: "blue"
        }
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 0
        focus: true
        currentIndex: -1

        property string currentDriveName: ""


    }

    Component {
        id: driveCell

        Item {
            id: driveCellItem
            width: driveGrid.cellWidth
            height: driveGrid.cellHeight

            Gradient {
                id: highlightGradient
                GradientStop {
                    position: 0
                    color: "#00aaff"
                }

                GradientStop {
                    position: 1
                    color: "#7ed6ff"
                }
            }

            Rectangle {
                id: driveCellItemPanel
                anchors.fill: parent
                color: "transparent"

                state: "normal"
                states: [
                    State {
                        name: "highlight"
                        PropertyChanges {
                            target: driveCellItemPanel
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
                    }

                    Column {
                        width: parent.width - driveIcon.width - 4
                        height: parent.height
                        spacing: 0
                        Text {
                            width: parent.width
                            height: 29
                            color: "white"
                            font.family: "Century Gothic"
                            font.pixelSize: 18
                            text: model.logicalDrive + ": " + driveTypeTexts[model.driveType]
                        }

                        Rectangle {
                            width: parent.width
                            height: 29
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
                                font.family: "Century Gothic"
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                style: Text.Outline
                                font.pixelSize:16
                                text: "Free: " + Utility.formatFileSize(model.availableSpace, 1)
                                      + " / Total: " + Utility.formatFileSize(model.totalSpace, 1)
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "grey"
                    anchors.bottom: parent.bottom
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton

                    onPressed: {
                        driveCellItemPanel.state = "highlight";
                    }

                    onReleased: {
                        driveCellItemPanel.state = "normal";
                    }

                    onClicked: {
                        console.debug("driveCellItem clicked " + (parent.x + mouseX) + ", " + (parent.y + mouseY) );
                        var index = driveGrid.indexAt(parent.x + mouseX, parent.y + mouseY);
                        driveGrid.currentDriveName = model.logicalDrive;
                        driveGrid.currentIndex = index;
                        driveGridRect.driveSelected(model.logicalDrive);
                    }
                }
            }
        }
    }
}
