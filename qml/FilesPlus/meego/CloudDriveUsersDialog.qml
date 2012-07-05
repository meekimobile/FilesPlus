import QtQuick 1.1
import com.nokia.meego 1.1
import Charts 1.0
import FolderSizeItemListModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: uidDialog
    height: 200
    
    property int operation
    property string localPath
    property int selectedCloudType
    property string selectedUid
    property int selectedModelIndex

    signal opened()
    
    titleText: "Please select Cloud Account"
    delegate: Rectangle {
        id: uidDialogListViewItem
        height: 60
        Row {
            anchors.fill: uidDialogListViewItem.paddingItem
            spacing: 2
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: 30
                source: cloudDriveModel.getCloudIcon(type)
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 64
                height: parent.height
                font.pointSize: 18
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
                text: email
            }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: 30
                source: "cloud.svg"
                visible: (hash != "")
            }
        }
        
        MouseArea {
            anchors.fill: parent
            //TODO implement onPressed, onReleased to highlight item.
            onClicked: {
                uidDialog.selectedIndex = index;
                uidDialog.selectedCloudType = uidDialog.model.get(uidDialog.selectedIndex).type;
                uidDialog.selectedUid = uidDialog.model.get(uidDialog.selectedIndex).uid;
                uidDialog.accept();
            }
        }
    }
        
    onStatusChanged: {
        if (status == DialogStatus.Open) {
            opened();
        }
    }
}
