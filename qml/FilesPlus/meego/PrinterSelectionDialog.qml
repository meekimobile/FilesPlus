import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: printerSelectionDialog
    style: SelectionDialogStyle { dim: 0.9; pressDelay: 100 }

    property string srcFilePath
    property string srcURL
    
    titleText: (srcFilePath != "")
               ? appInfo.emptyStr+qsTr("Print %1 to").arg(fsModel.getFileName(srcFilePath))
               : appInfo.emptyStr+qsTr("Print URL to")

    onStatusChanged: {
//        console.debug("PrinterSelectionDialog onStatusChanged " + status + " model.count " + model.count);
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
        }
    }
}
