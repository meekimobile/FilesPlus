import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveJobsPage

    property string name: "cloudDriveJobsPage"
    property bool inverted: !theme.inverted
    property bool showBytes: false

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
            onClicked: {
                showBytes = !showBytes;
            }

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
            buttonIconSource: "toolbar-view-menu"
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
        id: jobStopMenu

        content: MenuLayout {
            id: jobStopMenuLayout

            // TODO Alias for fixing incorrect children.
            default property alias children: jobStopMenuLayout.menuChildren

            MenuItemWithIcon {
                name: "stop"
                text: appInfo.emptyStr+qsTr("Stop")
                onClicked: {
                    var jobId = jobListView.model.get(jobListView.currentIndex).jobId;
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
            id: menuLayout

            // TODO Alias for fixing incorrect children.
            default property alias children: menuLayout.menuChildren

            MenuItemWithIcon {
                name: "resume"
                text: appInfo.emptyStr+qsTr("Resume")
                onClicked: {
                    var jobId = jobListView.model.get(jobListView.currentIndex).jobId;
                    cloudDriveModel.resumeJob(jobId);
                }
            }

            MenuItemWithIcon {
                name: "remove"
                text: appInfo.emptyStr+qsTr("Remove")
                onClicked: {
                    var jobId = jobListView.model.get(jobListView.currentIndex).jobId;
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
            id: optionsMenuLayout

            // TODO Alias for fixing incorrect children.
            default property alias children: optionsMenuLayout.menuChildren

            MenuItemWithIcon {
                name: "resumeAll"
                text: appInfo.emptyStr+qsTr("Resume all")
                onClicked: {
                    for (var i=0; i<cloudDriveJobsModel.count; i++) {
                        var jobId = cloudDriveJobsModel.get(i).jobId;
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

            MenuItemWithIcon {
                name: "resetRunningJobCount"
                text: appInfo.emptyStr+qsTr("Reset stalled jobs")
                onClicked: {
                    resetRunningJobCountConfirmation.open();
                }
            }
        }
    }

    ConfirmDialog {
        id: resetRunningJobCountConfirmation
        titleText: appInfo.emptyStr+qsTr("Reset stalled jobs")
        contentText: appInfo.emptyStr+qsTr("Reset stalled jobs") + "?"
        onConfirm: {
            cloudDriveModel.resetRunningJobCount();
        }
    }

    Component {
        id: jobDelegate

        ListItem {
            id: listItem
            showPressed: false

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
                            visible: isRunning && !showBytes
                            value: bytes
                            minimumValue: 0
                            maximumValue: bytesTotal
                        }
                        Text {
                            text: appInfo.emptyStr+Utility.formatFileSize(bytes, 1)+" / "+Utility.formatFileSize(bytesTotal, 1)
                            width: parent.width
                            horizontalAlignment: Text.AlignRight
                            visible: isRunning && showBytes
                            font.pointSize: 14
                            color: (!inverted) ? "white" : "black"
                        }
                        Text {
                            text: appInfo.emptyStr+(err != 0 ? qsTr("Error %1 %2").arg(err).arg(errString) : "")
                            width: parent.width
                            visible: !isRunning && !showBytes
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
                        source: cloudDriveModel.getCloudIcon(type)
                        width: 24
                        height: 24
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        id: sourceName
                        text: {
                            if (operation == CloudDriveModel.Quota) {
                                return uid + " (" + cloudDriveModel.getUidEmail(type, uid) + ")";
                            } else {
                                return remoteFilePath;
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
                            if (operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                return cloudDriveModel.getCloudIcon(targetType)
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
                        id: targetName
                        text: {
                            if (operation == CloudDriveModel.MigrateFile || operation == CloudDriveModel.MigrateFilePut) {
                                if (cloudDriveModel.isRemoteAbsolutePath(targetType)) {
                                    return cloudDriveModel.getRemotePath(targetType, newRemoteFilePath, newRemoteFileName);
                                } else {
                                    return newRemoteFilePath + " > " + newRemoteFileName;
                                }
                            } else {
                                return localFilePath;
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

            Timer {
                id: refreshTimer
                interval: 500
                repeat: true
                running: isRunning
                onTriggered: {
                    var jobJson = cloudDriveModel.getJobJson(jobId);
                    cloudDriveJobsModel.updateJob(jobJson);
                }
            }

            onClicked: {
                console.debug("cloudDriveJobsPage listItem onClicked jobJson " + JSON.stringify(cloudDriveModel.getJobJson(jobId)) );
                if (!isRunning) {
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
