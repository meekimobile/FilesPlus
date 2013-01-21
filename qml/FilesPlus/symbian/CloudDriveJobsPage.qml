import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveJobsPage

    property string name: "cloudDriveJobsPage"
    property bool inverted: window.platformInverted

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        // Update (runningJobCount + jobQueueCount) on cloudButton.
        cloudJobsCountIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            cloudDriveModel.requestJobQueueStatus();
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
//        ToolButton {
//            id: controlButton

//            property bool showPlay: false

//            iconSource: (showPlay) ? "toolbar-mediacontrol-play" : "toolbar-mediacontrol-pause"
//            platformInverted: window.platformInverted
//            flat: true
//            onClicked: {
//                if (showPlay) {
//                    cloudDriveModel.resumeNextJob();
//                } else {
//                    cloudDriveModel.suspendNextJob();
//                }
//                showPlay = !showPlay;
//            }
//        }
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

    ConfirmDialog {
        id: resumeJobConfirmation

        property string jobId

        titleText: appInfo.emptyStr+qsTr("Resume job")
        contentText: appInfo.emptyStr+qsTr("Resume job %1?").arg(jobId);
        onConfirm: {
            cloudDriveModel.resumeJob(jobId);
        }
    }

    Component {
        id: jobDelegate

        ListItem {
            id: listItem

            property int mouseX
            property int mouseY
            property bool showBytes: false

            Column {
                width: parent.width
                anchors.centerIn: parent
                Row {
                    width: parent.width
                    Image {
                        id: runningIcon
                        width: 22
                        height: 22
                        source: fsModel.getRunningOperationIconSource(fsModel.mapToFolderSizeListModelOperation(operation))
                    }
                    Text {
                        text: cloudDriveModel.getOperationName(operation)
                        width: parent.width - runningIcon.width - syncProgressBar.width
                        anchors.verticalCenter: parent.verticalCenter
                        font.pointSize: 6
                        elide: Text.ElideRight
                        color: (!inverted) ? "white" : "black"
                    }
                    ProgressBar {
                        id: syncProgressBar
                        width: parent.width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        visible: is_running && !listItem.showBytes
                        value: bytes
                        minimumValue: 0
                        maximumValue: bytes_total
                    }
                    Text {
                        text: appInfo.emptyStr+Utility.formatFileSize(bytes, 1)+" / "+Utility.formatFileSize(bytes_total, 1)
                        width: parent.width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignRight
                        visible: listItem.showBytes
                        font.pointSize: 6
                        color: (!inverted) ? "white" : "black"
                    }
                    Text {
                        text: appInfo.emptyStr+qsTr("Error %1 %2").arg(err).arg(err_string)
                        width: parent.width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !is_running && !listItem.showBytes
                        font.pointSize: 6
                        elide: Text.ElideRight
                        color: (!inverted) ? "white" : "black"
                    }
                }
                Row {
                    width: parent.width
                    Image {
                        id: sourceCloudIcon
                        source: {
                            if (operation == CloudDriveModel.FileGet || operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                return cloudDriveModel.getCloudIcon(type);
                            } else {
                                return "";
                            }
                        }
                        width: 22
                        height: 22
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        id: sourceName
                        text: {
                            if (operation == CloudDriveModel.FileGet || operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                return remote_file_path;
                            } else {
                                return local_file_path;
                            }
                        }
                        width: parent.width - sourceCloudIcon.width
                        font.pointSize: 6
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Image {
                        id: targetCloudIcon
                        source: {
                            if (operation == CloudDriveModel.FileGet) {
                                return "";
                            } else if (operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                return cloudDriveModel.getCloudIcon(target_type)
                            } else {
                                return cloudDriveModel.getCloudIcon(type)
                            }
                        }
                        width: 22
                        height: 22
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        id: targetName
                        text: {
                            if (operation == CloudDriveModel.FileGet) {
                                return local_file_path;
                            } else if (operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                return new_remote_file_name;
                            } else {
                                return remote_file_path;
                            }
                        }
                        width: parent.width - targetCloudIcon.width
                        font.pointSize: 6
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            onClicked: {
                console.debug("cloudDriveJobsPage listItem onClicked jobJson " + cloudDriveModel.getJobJson(job_id) );
                showBytes = !showBytes;
            }

            onPressAndHold: {
                console.debug("cloudDriveJobsPage listItem onPressAndHold jobJson " + cloudDriveModel.getJobJson(job_id) );
                resumeJobConfirmation.jobId = job_id;
                resumeJobConfirmation.open();
            }
        }
    }
}
