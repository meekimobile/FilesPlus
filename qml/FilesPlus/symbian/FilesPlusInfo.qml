import QtQuick 1.1

Rectangle {
    id: background
    anchors.fill: parent
    color: "black"

    property string version

    states: [
        State {
            name: "portrait"
            when: window.inPortrait
            PropertyChanges {
                target: info1
                width: parent.width
            }
            AnchorChanges {
                target: info1
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: undefined
            }
            PropertyChanges {
                target: info2
                width: parent.width
            }
            AnchorChanges {
                target: info2
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: undefined
            }
        },
        State {
            name: "landscape"
            when: !window.inPortrait
            PropertyChanges {
                target: info1
                width: parent.width / 2
            }
            AnchorChanges {
                target: info1
                anchors.horizontalCenter: undefined
                anchors.verticalCenter: parent.verticalCenter
            }
            PropertyChanges {
                target: info2
                width: parent.width / 2
            }
            AnchorChanges {
                target: info2
                anchors.horizontalCenter: undefined
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    ]

    Flow {
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter

    Column {
        id: info1

        Image {
            id: icon
            width: 120
            height: 120
            anchors.horizontalCenter: parent.horizontalCenter
            source: "FilesPlus256.png"
            smooth: true

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    Qt.openUrlExternally("http://www.meeki.mobi/products/filesplus");
                }
            }
        }

        Text {
            id: title
            color: "white"
            text: appInfo.emptyStr+qsTr("FilesPlus" + ((version=="")?"":(" "+version)))
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignLeft
            font.bold: true
            font.pointSize: 10
        }
    }

    Column {
        id: info2

        Text {
            id: description
            color: "grey"
            width: 300
            text: appInfo.emptyStr+qsTr("FilesPlus provides extended functions beyond file manager.\
\n   + Print with Google Cloud Print.\
\n   + Sync with multiple cloud storages.\
\n   + Preview images in your folder.\
\n   + Present folders in Pie view.")
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.WordWrap
            font.pointSize: 6
        }

        Row {
            height: 80
            anchors.horizontalCenter: parent.horizontalCenter
            Image {
                source: "dropbox_icon.png"
                anchors.verticalCenter: parent.verticalCenter
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        Qt.openUrlExternally("https://www.dropbox.com");
                    }
                }
            }
            Image {
                source: "skydrive_icon.png"
                anchors.verticalCenter: parent.verticalCenter
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        Qt.openUrlExternally("https://skydrive.live.com/");
                    }
                }
            }
            Image {
                source: "drive_icon.png"
                anchors.verticalCenter: parent.verticalCenter
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        Qt.openUrlExternally("https://drive.google.com");
                    }
                }
            }
            Image {
                source: "ftp_icon.png"
                anchors.verticalCenter: parent.verticalCenter
            }
            Image {
                source: "webdav_icon.png"
                anchors.verticalCenter: parent.verticalCenter
            }
            Image {
                source: "cloudprint-ready.png"
                anchors.verticalCenter: parent.verticalCenter
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        Qt.openUrlExternally("http://www.google.com/cloudprint/learn");
                    }
                }
            }
        }

        Text {
            id: author
            width: parent.width
            color: "grey"
            text: appInfo.emptyStr+qsTr("Developed by ") + "MeekiMobile"
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.WordWrap
            font.pointSize: 6
        }

        Image {
            source: "MeekiMobile256.svg"
            width: 48
            height: width
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    Qt.openUrlExternally("http://www.meeki.mobi/");
                }
            }
        }
    }
    }
}
