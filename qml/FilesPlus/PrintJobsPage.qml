import QtQuick 1.1
import com.nokia.symbian 1.1
import GCPClient 1.0
import "Utility.js" as Utility

Page {
    id: printJobsPage

    property string name: "printJobsPage"

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            flat: true
            onClicked: {
                pageStack.pop();
            }
        }

        ToolButton {
            id: refreshButton
            iconSource: "toolbar-refresh"
            flat: true
            onClicked: {
                gcpClient.jobs("");
            }
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
    }

    function addJobsFromJson(msg) {
        var jsonObj = Utility.createJsonObj(msg);
        console.debug("jsonObj.jobs " + jsonObj.jobs + " " + jsonObj.jobs.length);
        for (var i=0; i<jsonObj.jobs.length; i++)
        {
            var job = jsonObj.jobs[i];
            jobModel.append({
                             id: job.id,
                             printerid: job.printerid,
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
    }

    TitlePanel {
        id: titlePanel
        text: "Print Jobs"
    }

    ListModel {
        id: jobModel
    }

    ListView {
        id: jobListView
        width: parent.width
        height: parent.height - titlePanel.height
        model: jobModel
        delegate: jobDelegate
    }

    Component {
        id: jobDelegate

        ListItem {
                id: jobItem
                Row {
//                    anchors.fill: parent.paddingItem
//                    spacing: 5

                    Column {
                        width: parent.width - 72
                        ListItemText {
                            mode: listItem.mode
                            role: "Title"
                            text: id + " " + title
                            width: parent.width
                            verticalAlignment: Text.AlignVCenter
                        }
                        ListItemText {
                            mode: listItem.mode
                            role: "SubTitle"
                            text: printerid
                            width: parent.width
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    ListItemText {
                        mode: listItem.mode
                        role: "Subtitle"
                        text: status
                        width: 72
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }

        }
    }
}
