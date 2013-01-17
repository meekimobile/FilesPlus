import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveJobsPage

    property string name: "cloudDriveJobsPage"
    property bool inverted: !theme.inverted

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

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
//        ToolIcon {
//            id: controlButton

//            property bool showPlay: false

//            iconId: (showPlay) ? "toolbar-mediacontrol-play" : "toolbar-mediacontrol-pause"
//            onClicked: {
//                if (showPlay) {
//                    cloudDriveModel.resumeNextJob();
//                } else {
//                    cloudDriveModel.suspendNextJob();
//                }
//                showPlay = !showPlay;
//            }
//        }
        ToolIcon {
            id: deleteAllButton
            iconSource: (!inverted) ? "delete.svg" : "delete_inverted.svg"
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
            id: listItem

            Column {
                width: parent.width
                anchors.centerIn: parent
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
                        anchors.verticalCenter: parent.verticalCenter
                        font.pointSize: 14
                        elide: Text.ElideRight
                        color: (!inverted) ? "white" : "black"
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
                        width: 24
                        height: 24
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
                        font.pointSize: 14
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
                        width: 24
                        height: 24
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
                        font.pointSize: 14
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            onClicked: {
                console.debug("cloudDriveJobsPage listItem onClicked jobJson " + cloudDriveModel.getJobJson(job_id) );
            }
        }
    }
}
