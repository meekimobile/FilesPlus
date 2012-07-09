import QtQuick 1.1
import com.nokia.meego 1.1
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
            iconSource: "delete.svg"
            onClicked: {
                deleteConfirmation.open();
            }
        }
    }

    ConfirmDialog {
        id: deleteConfirmation
        titleText: "Delete print jobs"
        contentText: "Delete all print jobs ?"
        onConfirm: {
            deleteAllJobs();
        }
    }

    GCPClient {
        id: gcpClient

        property string selectedFilePath

        onJobsReplySignal: {
            console.debug("printJobsPage gcpClient onJobsReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                addJobsFromJson(msg);
            }
        }

        onDeletejobReplySignal: {
            console.debug("printJobsPage gcpClient onDeletejobReplySignal " + err + " " + errMsg + " " + msg);

            gcpClient.jobs("");
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
                jobModel.setProperty(i, "status", "Deleting");
                gcpClient.deletejob(jobId);
            }
        }
    }

    function deleteAllJobs() {
        for (var i=0; i<jobModel.count; i++) {
            var jobId = jobModel.get(i).id;
            jobModel.setProperty(i, "status", "Deleting");
            gcpClient.deletejob(jobId);
        }
    }

    TitlePanel {
        id: titlePanel
        text: "Print Jobs"
    }

    ListModel {
        id: jobModel
    }

    Button {
        id: popupDeleteButton

        property string jobId

        iconSource: "delete.svg"
        visible: false
        width: 60
        height: 60
        z: 2
        onClicked: {
            // Delete selected job.
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
        model: jobModel
        delegate: jobDelegate

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
        }
    }

    Component {
        id: jobDelegate

        ListItem {
            id: jobListItem
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
                        color: "white"
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
                    width: 120
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    font.pointSize: 16
                    elide: Text.ElideMiddle
                    color: "grey"
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
        }
    }

    Component.onCompleted: {
        gcpClient.jobs("");
    }
}
