import QtQuick 1.1

Column {
    id: filePlusInfo
    width: parent.width
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter

    property string version

    Image {
        id: icon
        width: 180
        height: 180
        anchors.horizontalCenter: parent.horizontalCenter
        source: "FilesPlus256.png"

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                Qt.openUrlExternally("https://sites.google.com/site/meekimobile/products/filesplus");
            }
        }
    }

    Text {
        id: title
        color: "white"
        text: qsTr("FilesPlus" + ((version=="")?"":(" "+version)))
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignLeft
        font.bold: true
        font.pointSize: 10
    }

    Text {
        id: description
        color: "grey"
        width: 300
        text: "FilesPlus provide extended functions beyond file manager.\
\n   • Print with Google™ Cloud Print.\
\n   • Sync with Cloud Drive.\
\n   • Preview images in your folder.\
\n   • Present folders in Pie view."
        anchors.horizontalCenter: parent.horizontalCenter
        wrapMode: Text.WordWrap
        font.pointSize: 6
    }

    Row {
        height: 80
        anchors.horizontalCenter: parent.horizontalCenter
        Image {
            source: "dropbox_white.png"
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
        text: qsTr("Developed by MeekiMobile")
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
                Qt.openUrlExternally("http://sites.google.com/site/meekimobile");
            }
        }
    }
}
