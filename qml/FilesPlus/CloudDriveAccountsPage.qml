import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveAccountsPage

    property string name: "cloudDriveAccountsPage"
    property variant cloudDriveModel
    property alias accountModel: accountListView.model

    function getCloudIcon(type) {
        switch (type) {
        case CloudDriveModel.Dropbox:
            return "dropbox_icon.png";
        case CloudDriveModel.GoogleDrive:
            return "drive_icon.png";
        }
    }

    function getCloudName(type) {
        switch (type) {
        case CloudDriveModel.Dropbox:
            return "Dropbox";
        case CloudDriveModel.GoogleDrive:
            return "GoogleDrive";
        }
    }

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            flat: true
            onClicked: {
                pageStack.pop();
            }
        }

        ToolButton {
            id: refreshButton
            iconSource: "toolbar-refresh"
            flat: true
            onClicked: {
            }
        }

        ToolButton {
            id: menuButton
            iconSource: "toolbar-menu"
            flat: true
            onClicked: {
            }
        }
    }

//    function addJobsFromJson(msg) {
//        jobModel.clear();

//        var jsonObj = Utility.createJsonObj(msg);
//        console.debug("jsonObj.jobs " + jsonObj.jobs + " " + jsonObj.jobs.length);
//        for (var i=0; i<jsonObj.jobs.length; i++)
//        {
//            var job = jsonObj.jobs[i];
//            jobModel.append({
//                                id: job.id,
//                                printerid: job.printerid,
//                                printerName: job.printerName,
//                                title: job.title,
//                                contentType: job.contentType,
//                                fileUrl: job.fileUrl,
//                                ticketUrl: job.ticketUrl,
//                                createTime: job.createTime,
//                                updateTime: job.updateTime,
//                                status: job.status,
//                                errorCode: job.errorCode,
//                                message: job.message,
//                                tags: job.tags
//                            });
//        }

//        jobListView.model = jobModel;
//    }

//    function deleteAllDoneJobs() {
//        for (var i=0; i<jobModel.count; i++) {
//            var jobId = jobModel.get(i).id;
//            var status = jobModel.get(i).status;
//            if (status == "DONE") {
//                gcpClient.deletejob(jobId);
//            }
//        }
//    }

    TitlePanel {
        id: titlePanel
        text: "Cloud Drive Accounts"
    }

    ListModel {
        id: accountModel
    }

    Button {
        id: popupDeleteButton

        property string uid

        iconSource: "delete.svg"
        visible: false
        width: 50
        height: 50
        z: 2
        onClicked: {
            // TODO delete selected account.
            accountModel.setProperty(jobListView.currentIndex, "status", "Deleting");
            visible = false;
        }
        onVisibleChanged: {
            if (visible) popupDeleteButtonTimer.restart();
        }

        Timer {
            id: popupDeleteButtonTimer
            interval: 2000
            running: false
            onTriggered: {
                parent.visible = false;
            }
        }
    }

    ListView {
        id: accountListView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: accountModel
        delegate: accountDelegate
    }

    Component {
        id: accountDelegate

        ListItem {
            id: listItem

            property int mouseX
            property int mouseY

            Row {
                anchors.fill: parent.paddingItem
                spacing: 5

                Image {
                    id: cloudIcon
                    source: getCloudIcon(type)
                }
                Column {
                    width: parent.width - cloudIcon.width
                    ListItemText {
                        mode: listItem.mode
                        role: "Title"
                        text: email
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                    }
                    Row {
                        ListItemText {
                            mode: listItem.mode
                            role: "SubTitle"
                            text: "UID " + uid
                            width: parent.width - quotaText.width
                            verticalAlignment: Text.AlignVCenter
                        }
                        ListItemText {
                            id: quotaText
                            mode: listItem.mode
                            role: "Subtitle"
                            text: normal + "," + shared + " /" + quota
                            width: 120
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            onPressAndHold: {
                var panelX = x + mouseX - accountListView.contentX;
                var panelY = y + mouseY - accountListView.contentY + jobListView.y;
                popupDeleteButton.x = panelX - (popupDeleteButton.width / 2);
                popupDeleteButton.y = panelY - (popupDeleteButton.height);
                popupDeleteButton.uid = uid;
                popupDeleteButton.visible = true;
            }

            MouseArea {
                anchors.fill: parent
                onPressed: {
                    parent.mouseX = mouseX;
                    parent.mouseY = mouseY;
                    mouse.accepted = false;
                }
            }

            Component.onCompleted: {
                cloudDriveAccountsPage.cloudDriveModel.accountInfo(type, uid);
            }
        }
    }

    Component.onCompleted: {
    }
}
