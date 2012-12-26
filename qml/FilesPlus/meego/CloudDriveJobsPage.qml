import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveJobsPage

    property string name: "cloudDriveJobsPage"

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        // Update (runningJobCount + jobQueueCount) on cloudButton.
        cloudJobsCountIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
    }

    function updateItemSlot(jobJson) {
        if (!jobJson) return;

        var i = jobListView.model.findIndexByJobId(jobJson.job_id);
        if (i >= 0) {
            jobListView.model.set(i, { is_running: jobJson.is_running });
        }
    }

    function updateItemProgressBarSlot(jobJson) {
        if (!jobJson) return;

        var i = jobListView.model.findIndexByJobId(jobJson.job_id);
        if (i >= 0) {
            jobListView.model.set(i, { is_running: jobJson.is_running, bytes: jobJson.bytes, bytes_total: jobJson.bytes_total });
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
        }
    }

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
            }
        }
        ToolIcon {
            id: deleteAllButton
            iconSource: (theme.inverted) ? "delete.svg" : "delete_inverted.svg"
            onClicked: {
                cloudDriveModel.jobsModel.clear();
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Cloud Drive Jobs")

        TextIndicator {
            id: cloudJobsCountIndicator
            color: "#00AAFF"
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Button {
        id: popupDeleteButton

        property string jobId

        iconSource: (theme.inverted) ? "delete.svg" : "delete_inverted.svg"
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
        model: cloudDriveModel.jobsModel
        delegate: jobDelegate
        clip: true

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
        }
    }

    Component {
        id: jobDelegate

        ListItem {
            id: listItem
            height: 80

            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Image {
                    id: cloudIcon
                    source: cloudDriveModel.getCloudIcon(type)
                    width: 48
                    height: 48
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    width: parent.width - cloudIcon.width
                    Text {
                        id: title
                        text: local_file_path
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: (theme.inverted) ? "white" : "black"
                    }
                    Row {
                        width: parent.width
                        Image {
                            id: runningIcon
                            width: 24
                            height: 24
                            source: fsModel.getRunningOperationIconSource(fsModel.mapToFolderSizeListModelOperation(operation))
                        }
                        Text {
                            text: cloudDriveModel.getOperationName(operation)
                            width: parent.width - runningIcon.width - syncProgressBar.width
                            verticalAlignment: Text.AlignVCenter
                            font.pointSize: 16
                            elide: Text.ElideRight
                            color: "grey"
                        }
                        ProgressBar {
                            id: syncProgressBar
                            width: parent.width / 2
                            anchors.verticalCenter: parent.verticalCenter
//                            indeterminate: is_running
                            visible: is_running
                            value: bytes
                            minimumValue: 0
                            maximumValue: bytes_total
                        }
                    }
                }
            }

            onPressAndHold: {
//                removeAccountConfirmation.index = index;
//                removeAccountConfirmation.open();
            }
        }
    }
}
