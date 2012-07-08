import QtQuick 1.1
import com.nokia.meego 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: uidDialog
    style: SelectionDialogStyle { dim: 0.9 }

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
            text += "Sync " + fsModel.getFileName(localPath) + " to";
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
            titleText = getTitleText(localPath);
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        }
    }
}
