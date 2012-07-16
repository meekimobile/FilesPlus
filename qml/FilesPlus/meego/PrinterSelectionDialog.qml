import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: printerSelectionDialog
    style: SelectionDialogStyle { dim: 0.9 }

    property string srcFilePath
    
    titleText: qsTr("Print") + " " + fsModel.getFileName(srcFilePath) + " " + qsTr("to")
//    titleIcon: "FilesPlusIcon.svg"

    onStatusChanged: {
//        console.debug("PrinterSelectionDialog onStatusChanged " + status + " model.count " + model.count);
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
        }
    }
}
