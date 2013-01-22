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
        highlightRangeMode: ListView.NoHighlightRange
        highlightFollowsCurrentItem: false
        highlightMoveDuration: 1
        highlightMoveSpeed: 4000
        highlight: Rectangle {
            width: jobListView.width
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "#0080D0"
                }

                GradientStop {
                    position: 1
                    color: "#53A3E6"
                }
            }
        }

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

    MenuWithIcon {
        id: jobMenu

        content: MenuLayout {
            id: menuLayout

            // TODO Alias for fixing incorrect children.
            default property alias children: menuLayout.menuChildren

            MenuItemWithIcon {
                name: "resume"
                text: appInfo.emptyStr+qsTr("Resume")
                onClicked: {
                    var jobId = jobListView.model.get(jobListView.currentIndex).job_id;
                    cloudDriveModel.resumeJob(jobId);
                }
            }

            MenuItemWithIcon {
                name: "remove"
                text: appInfo.emptyStr+qsTr("Remove")
                onClicked: {
                    var jobId = jobListView.model.get(jobListView.currentIndex).job_id;
                    cloudDriveModel.removeJob("cloudDriveJobsPage jobMenu", jobId);
                }
            }
        }

        onClosed: {
            // Hide highlight.
            jobListView.currentIndex = -1;
            jobListView.highlightFollowsCurrentItem = false;
        }
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

            property bool showBytes: false

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
                    Column {
                        width: parent.width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        ProgressBar {
                            id: syncProgressBar
                            width: parent.width
                            visible: is_running && !listItem.showBytes
                            value: bytes
                            minimumValue: 0
                            maximumValue: bytes_total
                        }
                        Text {
                            text: appInfo.emptyStr+Utility.formatFileSize(bytes, 1)+" / "+Utility.formatFileSize(bytes_total, 1)
                            width: parent.width
                            horizontalAlignment: Text.AlignRight
                            visible: listItem.showBytes
                            font.pointSize: 14
                            color: (!inverted) ? "white" : "black"
                        }
                        Text {
                            text: appInfo.emptyStr+(err != 0 ? qsTr("Error %1 %2").arg(err).arg(err_string) : "")
                            width: parent.width
                            visible: !is_running && !listItem.showBytes
                            font.pointSize: 14
                            elide: Text.ElideRight
                            color: (!inverted) ? "white" : "black"
                        }
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
                showBytes = !showBytes;
            }

            onPressAndHold: {
                console.debug("cloudDriveJobsPage listItem onPressAndHold jobJson " + cloudDriveModel.getJobJson(job_id) );
                if (!is_running) {
//                    resumeJobConfirmation.jobId = job_id;
//                    resumeJobConfirmation.open();
                    jobListView.currentIndex = index;
                    jobListView.highlightFollowsCurrentItem = true;
                    jobMenu.open();
                }
            }
        }
    }
}
