import QtQuick 1.1
import com.nokia.meego 1.0
import GCPClient 1.0
import "Utility.js" as Utility

Page {
    id: printJobsPage

    property string name: "printJobsPage"

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }

        ToolIcon {
            id: refreshButton
            iconId: "toolbar-refresh"
            onClicked: {
                gcpClient.jobs("");
            }
        }

        ToolIcon {
            id: deleteButton
            iconSource: (theme.inverted) ? "delete.svg" : "delete_inverted.svg"
            onClicked: {
                deleteConfirmation.open();
            }
        }
    }

    ConfirmDialog {
        id: deleteJobConfirmation

        property string jobId
        property int jobIndex

        titleText: appInfo.emptyStr+qsTr("Delete")
        contentText: appInfo.emptyStr+qsTr("Delete %1 ?").arg(jobId);
        onConfirm: {
            deleteJob(jobId, jobIndex);
        }
    }

    ConfirmDialog {
        id: deleteConfirmation
        titleText: appInfo.emptyStr+qsTr("Delete print jobs")
        contentText: appInfo.emptyStr+qsTr("Delete all print jobs ?")
        onConfirm: {
            deleteAllJobs();
        }
    }

    function addJobsFromJson(msg) {
        jobModel.clear();

        var jsonObj = Utility.createJsonObj(msg);
        console.debug("jsonObj.jobs " + jsonObj.jobs + " " + jsonObj.jobs.length);
        for (var i=0; i<jsonObj.jobs.length; i++)
        {
            var job = jsonObj.jobs[i];
            jobModel.append({
                                id: job.id,
                                printerid: job.printerid,
                                printerName: job.printerName,
                                title: job.title,
                                contentType: job.contentType,
                                fileUrl: job.fileUrl,
                                ticketUrl: job.ticketUrl,
                                createTime: job.createTime,
                                updateTime: job.updateTime,
                                status: job.status,
                                errorCode: job.errorCode,
                                message: job.message,
                                tags: job.tags
                            });
        }

        jobListView.model = jobModel;
    }

    function deleteAllDoneJobs() {
        for (var i=0; i<jobModel.count; i++) {
            var jobId = jobModel.get(i).id;
            var status = jobModel.get(i).status;
            if (status == "DONE") {
                jobModel.setProperty(i, "status", appInfo.emptyStr+qsTr("Deleting"));
                gcpClient.deletejob(jobId);
            }
        }
    }

    function deleteAllJobs() {
        for (var i=0; i<jobModel.count; i++) {
            var jobId = jobModel.get(i).id;
            jobModel.setProperty(i, "status", appInfo.emptyStr+qsTr("Deleting"));
            gcpClient.deletejob(jobId);
        }
    }

    function deleteJob(jobId, index) {
        // Delete selected job.
        jobModel.setProperty(index, "status", appInfo.emptyStr+qsTr("Deleting"));
        gcpClient.deletejob(jobId);
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Print Jobs")
    }

    ListModel {
        id: jobModel
    }

    Button {
        id: popupDeleteButton

        property string jobId

        iconSource: (theme.inverted) ? "delete.svg" : "delete_inverted.svg"
        visible: false
        width: 60
        height: 60
        z: 2
        onClicked: {
            // Delete selected job.
            jobModel.setProperty(jobListView.currentIndex, "status", appInfo.emptyStr+qsTr("Deleting"));
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
        model: jobModel
        delegate: jobDelegate
        clip: true

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
        }
    }

    ScrollDecorator {
        id: scrollbar
        flickableItem: jobListView
    }

    Component {
        id: jobDelegate

        ListItem {
            id: jobListItem
            height: 80
            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Column {
                    width: parent.width - statusText.width - parent.spacing
                    Text {
                        text: title
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: (theme.inverted) ? "white" : "black"
                    }
                    Text {
                        text: printerName
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 16
                        elide: Text.ElideMiddle
                        color: "grey"
                    }
                }
                Text {
                    id: statusText
                    text: status
                    width: 160
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    font.pointSize: 16
                    elide: Text.ElideMiddle
                    color: "grey"
                }
            }

            onPressAndHold: {
//                var panelX = x + mouseX - jobListView.contentX;
//                var panelY = y + mouseY - jobListView.contentY + jobListView.y;
//                popupDeleteButton.x = panelX - (popupDeleteButton.width / 2);
//                popupDeleteButton.y = panelY - (popupDeleteButton.height);
//                popupDeleteButton.jobId = id;
//                popupDeleteButton.visible = true;
                deleteJobConfirmation.jobId = id;
                deleteJobConfirmation.jobIndex = index;
                deleteJobConfirmation.open();
            }
        }
    }

    Component.onCompleted: {
        gcpClient.jobs("");
    }
}
