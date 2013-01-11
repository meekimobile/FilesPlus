import QtQuick 1.1
import com.nokia.symbian 1.1
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
            id: deleteAllButton
            iconSource: (!window.platformInverted) ? "delete.svg" : "delete_inverted.svg"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                cancelQueuedCloudDriveJobsConfirmation.open();
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
        model: cloudDriveModel.jobsModel
        delegate: jobDelegate
        clip: true
    }

    ScrollDecorator {
        id: scrollbar
        flickableItem: jobListView
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
                    spacing: 5
                    Text {
                        id: title
                        text: local_file_path
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 8
                        elide: Text.ElideMiddle
                        color: (!window.platformInverted) ? "white" : "black"
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
                            font.pointSize: 6
                            elide: Text.ElideRight
                            color: "grey"
                        }
                        ProgressBar {
                            id: syncProgressBar
                            width: parent.width / 2
                            anchors.verticalCenter: parent.verticalCenter
                            visible: is_running
                            value: bytes
                            minimumValue: 0
                            maximumValue: bytes_total
                        }
                    }
                }
            }

            onClicked: {
                console.debug("cloudDriveJobsPage listItem onClicked jobJson " + cloudDriveModel.getJobJson(job_id) );
            }
        }
    }
}
