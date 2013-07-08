import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveJobsPage

    property string name: "cloudDriveJobsPage"
    property bool inverted: window.platformInverted

    onStatusChanged: {
        if (status == PageStatus.Active) {
            cloudDriveModel.requestJobQueueStatus();
        }
    }

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
            id: cloudButton
            buttonIconSource: (!inverted) ? "cloud.svg" : "cloud_inverted.svg"

            TextIndicator {
                id: cloudButtonIndicator
                text: (cloudDriveModel.jobCount > 0) ? cloudDriveModel.jobCount : ""
                color: ((cloudDriveModel.runningJobCount + cloudDriveModel.queuedJobCount) > 0) ? "#00AAFF" : "red"
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
            }
        }
        ToolBarButton {
            id: optionsButton
            buttonIconSource: "toolbar-menu"
            onClicked: {
                optionsMenu.open();
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Cloud Drive Jobs")
    }

    ListView {
        id: jobListView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: cloudDriveJobsModel
        delegate: jobDelegate
        clip: true
        pressDelay: 100
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
    }

    ScrollDecorator {
        id: scrollbar
        flickableItem: jobListView
    }

    MenuWithIcon {
        id: jobStopMenu

        content: MenuLayout {
            MenuItemWithIcon {
                name: "stop"
                text: appInfo.emptyStr+qsTr("Stop")
                onClicked: {
                    var jobId = jobListView.model.get(jobListView.currentIndex).job_id;
                    cloudDriveModel.suspendJob(jobId);
                }
            }
        }

        onClosed: {
            // Hide highlight.
            jobListView.currentIndex = -1;
            jobListView.highlightFollowsCurrentItem = false;
        }
    }

    MenuWithIcon {
        id: jobMenu

        content: MenuLayout {
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

    MenuWithIcon {
        id: optionsMenu

        content: MenuLayout {
            MenuItemWithIcon {
                name: "resumeAll"
                text: appInfo.emptyStr+qsTr("Resume all")
                onClicked: {
                    for (var i=0; i<cloudDriveJobsModel.count; i++) {
                        var jobId = cloudDriveJobsModel.get(i).job_id;
                        cloudDriveModel.resumeJob(jobId);
                    }
                }
            }

            MenuItemWithIcon {
                name: "removeAll"
                text: appInfo.emptyStr+qsTr("Remove all")
                onClicked: {
                    cancelQueuedCloudDriveJobsConfirmation.open();
                }
            }
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
                            font.pointSize: 6
                            color: (!inverted) ? "white" : "black"
                        }
                        Text {
                            text: appInfo.emptyStr+(err != 0 ? qsTr("Error %1 %2").arg(err).arg(err_string) : "")
                            width: parent.width
                            visible: !is_running && !listItem.showBytes
                            font.pointSize: 6
                            elide: Text.ElideRight
                            color: (!inverted) ? "white" : "black"
                        }
                    }
                }
                Row {
                    width: parent.width
                    Image {
                        id: sourceCloudIcon
                        source: cloudDriveModel.getCloudIcon(type)
                        width: 22
                        height: 22
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        id: sourceName
                        text: {
                            if (operation == CloudDriveModel.Quota) {
                                return uid + " (" + cloudDriveModel.getUidEmail(type, uid) + ")";
                            } else {
                                return remote_file_path;
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
                            if (operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                return cloudDriveModel.getCloudIcon(target_type)
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
                        id: targetName
                        text: {
                            if (operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                if (cloudDriveModel.isRemoteAbsolutePath(target_type)) {
                                    return cloudDriveModel.getRemotePath(target_type, new_remote_file_path, new_remote_file_name);
                                } else {
                                    return new_remote_file_path + " > " + new_remote_file_name;
                                }
                            } else {
                                return local_file_path;
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

            Timer {
                id: refreshTimer
                interval: 500
                repeat: true
                running: is_running
                onTriggered: {
                    var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(job_id));
                    cloudDriveJobsModel.updateJob(jobJson);
                }
            }

            onClicked: {
                console.debug("cloudDriveJobsPage listItem onClicked jobJson " + cloudDriveModel.getJobJson(job_id) );
                showBytes = !showBytes;
            }

            onPressAndHold: {
                console.debug("cloudDriveJobsPage listItem onPressAndHold jobJson " + cloudDriveModel.getJobJson(job_id) );
                if (!is_running) {
                    jobListView.currentIndex = index;
                    jobListView.highlightFollowsCurrentItem = true;
                    jobMenu.open();
                } else {
                    jobListView.currentIndex = index;
                    jobListView.highlightFollowsCurrentItem = true;
                    jobStopMenu.open();
                }
            }
        }
    }
}
