import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: printerSelectionDialog
    
    property string srcFilePath
    
    titleText: appInfo.emptyStr+qsTr("Print %1 to").arg(fsModel.getFileName(srcFilePath))
    titleIcon: "FilesPlusIcon.svg"
    
    onStatusChanged: {
//        console.debug("PrinterSelectionDialog onStatusChanged " + status + " model.count " + model.count);
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
        }
    }
}
