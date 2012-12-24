import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveJobsPage

    property string name: "cloudDriveJobsPage"

    function updateCloudDriveJobSlot(jobId, isRunning) {
        for (var i=0; i<jobListView.model.count; i++) {
            if (jobListView.model.get(i).job_id == jobId) {
                console.debug("cloudDriveJobsPage updateCloudDriveJobSlot " + jobId + " " + isRunning);
                jobListView.model.set(i, { is_running: isRunning });
                return i;
            }
        }

        return -1;
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
                            indeterminate: is_running
                            visible: is_running
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
