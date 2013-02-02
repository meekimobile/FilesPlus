import QtQuick 1.1
import com.nokia.meego 1.0
import GCPClient 1.0
import "Utility.js" as Utility

Page {
    id: printJobsPage

    property string name: "printJobsPage"
    property bool inverted: !theme.inverted

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }

        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            onClicked: {
                gcpClient.jobs("");
            }
        }

        ToolBarButton {
            id: deleteButton
            buttonIconSource: (!inverted) ? "delete.svg" : "delete_inverted.svg"
            onClicked: {
                deleteConfirmation.open();
            }
        }
    }

    ConfirmDialog {
        id: deleteConfirmation
        titleText: appInfo.emptyStr+qsTr("Deleting")
        contentText: appInfo.emptyStr+qsTr("Delete all print jobs ?")
        onConfirm: {
            deleteAllJobs();
        }
    }

    ConfirmDialog {
        id: deleteJobConfirmation
        titleText: appInfo.emptyStr+qsTr("Deleting")
        contentText: appInfo.emptyStr+qsTr("Delete print job %1\ntitle %2 ?").arg(jobModel.get(jobListView.currentIndex).id).arg(jobModel.get(jobListView.currentIndex).title)
        onConfirm: {
            // Delete selected job.
            var jobId = jobModel.get(jobListView.currentIndex).id;
            jobModel.setProperty(jobListView.currentIndex, "status", appInfo.emptyStr+qsTr("Deleting"));
            gcpClient.deletejob(jobId);
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
        var jobCount = jobModel.count;
        for (var i=0; i<jobCount; i++) {
            var jobId = jobModel.get(i).id;
            jobModel.setProperty(i, "status", appInfo.emptyStr+qsTr("Deleting"));
            gcpClient.deletejob(jobId, i == (jobCount-1));
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Print Jobs")
    }

    ListModel {
        id: jobModel
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
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                    }
                    Text {
                        text: printerName
                        width: parent.width
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
                    font.pointSize: 16
                    elide: Text.ElideMiddle
                    color: "grey"
                }
            }

            onPressAndHold: {
                deleteJobConfirmation.open();
            }
        }
    }

    Component.onCompleted: {
        gcpClient.jobs("");
    }
}
