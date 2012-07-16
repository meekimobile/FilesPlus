import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: printerSelectionDialog
    
    property string srcFilePath
    
    titleText: qsTr("Print") + " " + fsModel.getFileName(srcFilePath) + " " + qsTr("to")
    titleIcon: "FilesPlusIcon.svg"
    
    onStatusChanged: {
//        console.debug("PrinterSelectionDialog onStatusChanged " + status + " model.count " + model.count);
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
        }
    }
}
