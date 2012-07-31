import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Page {
    id: cloudDriveJobsPage

    property string name: "cloudDriveJobsPage"
    property alias jobModel: jobListView.model

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                pageStack.pop();
            }
        }

        ToolButton {
            id: refreshButton
            iconSource: "toolbar-refresh"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
            }
        }

        ToolButton {
            id: menuButton
            iconSource: "toolbar-menu"
            platformInverted: window.platformInverted
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
        text: appInfo.emptyStr+qsTr("Cloud Drive Jobs")
    }

    ListModel {
        id: jobModel
    }

    Button {
        id: popupDeleteButton

        property string jobId

        iconSource: (!window.platformInverted) ? "delete.svg" : "delete_inverted.svg"
        visible: false
        width: 50
        height: 50
        z: 2
        onClicked: {
            // TODO delete selected job.
            jobModel.setProperty(jobListView.currentIndex, "status", "Deleting");
            gcpClient.deletejob(jobId);
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
        id: jobListView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        delegate: jobDelegate
    }

    Component {
        id: jobDelegate

        ListItem {
            id: listItem

            property int mouseX
            property int mouseY

            Row {
                anchors.fill: parent.paddingItem
                spacing: 5

                Column {
                    width: parent.width - statusText.width - parent.spacing
                    ListItemText {
                        mode: listItem.mode
                        role: "Title"
                        text: jobId + " " + operation + " " + type + " " + uid
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        platformInverted: window.platformInverted
                    }
                    ListItemText {
                        mode: listItem.mode
                        role: "SubTitle"
                        text: localFilePath
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        platformInverted: window.platformInverted
                    }
                }
                ListItemText {
                    id: statusText
                    mode: listItem.mode
                    role: "Subtitle"
                    text: appInfo.emptyStr+(isRunning)?qsTr("Running"):qsTr("Queued")
                    width: 120
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    platformInverted: window.platformInverted
                }
            }

            onPressAndHold: {
                var panelX = x + mouseX - jobListView.contentX;
                var panelY = y + mouseY - jobListView.contentY + jobListView.y;
                popupDeleteButton.x = panelX - (popupDeleteButton.width / 2);
                popupDeleteButton.y = panelY - (popupDeleteButton.height);
                popupDeleteButton.jobId = id;
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
        }
    }

    Component.onCompleted: {
    }
}
