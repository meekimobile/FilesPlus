import QtQuick 1.1
import com.nokia.symbian 1.1
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

    signal opening()
    signal opened()
    
    function getTitleText(localPath) {
        var text = "";

        switch (operation) {
        case CloudDriveModel.Metadata:
            // TODO To support both single/multiple items.
            if (localPath != "") {
                text += "Sync " + fsModel.getFileName(localPath) + " to";
            } else if (clipboard.count > 0){
                var syncCount = 0;
                for (var i=0; i<clipboard.count; i++) {
                    if (clipboard.get(i).action == "sync") {
                        syncCount++;
                    }
                }
                text += "Sync " + syncCount + (syncCount>1 ? " items" : " item") + " to";
            }
            break;
        case CloudDriveModel.FilePut:
            text += "Upload " + fsModel.getFileName(localPath) + " to";
            break;
        case CloudDriveModel.FileGet:
            text += "Download " + fsModel.getFileName(localPath) + " from";
            break;
        case CloudDriveModel.ShareFile:
            text += "Share link of " + fsModel.getFileName(localPath) + " from";
            break;
        case CloudDriveModel.DeleteFile:
            text += "Unsync " + fsModel.getFileName(localPath) + " from";
            break;
        }

        return text;
    }

    delegate: ListItem {
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
            ListItemText {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 64
                mode: uidDialogListViewItem.mode
                role: "Title"
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
        
        onClicked: {
            uidDialog.selectedIndex = index;
            uidDialog.selectedCloudType = uidDialog.model.get(uidDialog.selectedIndex).type;
            uidDialog.selectedUid = uidDialog.model.get(uidDialog.selectedIndex).uid;
            uidDialog.accept();
        }
    }
        
    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
            titleText = getTitleText(localPath);
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        }
    }
}
