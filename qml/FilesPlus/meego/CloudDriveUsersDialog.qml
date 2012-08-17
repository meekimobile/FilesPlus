import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: uidDialog
    style: SelectionDialogStyle { dim: 0.9 }

    property string caller
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
                text += qsTr("Sync %1 to").arg(fsModel.getFileName(localPath));
            } else if (clipboard.count > 0){
                var syncCount = 0;
                for (var i=0; i<clipboard.count; i++) {
                    if (clipboard.get(i).action == "sync") {
                        syncCount++;
                    }
                }
                text += qsTr("Sync %n item(s) to", "disambiguation", syncCount);
            }
            break;
        case CloudDriveModel.FilePut:
            text += qsTr("Upload %1 to").arg(fsModel.getFileName(localPath));
            break;
        case CloudDriveModel.FileGet:
            text += qsTr("Download %1 from").arg(fsModel.getFileName(localPath));
            break;
        case CloudDriveModel.ShareFile:
            text += qsTr("Share link of %1 from").arg(fsModel.getFileName(localPath));
            break;
        case CloudDriveModel.DeleteFile:
            text += qsTr("Unsync %1 from").arg(fsModel.getFileName(localPath));
            break;
        }

        return text;
    }

    delegate: ListItem {
        id: uidDialogListViewItem
        showUnderline: false
        Row {
            anchors.fill: parent
            anchors.margins: 10
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
                font.pointSize: 18
                elide: Text.ElideMiddle
                color: "white"
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
            titleText = appInfo.emptyStr+getTitleText(localPath);
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        }
    }
}
