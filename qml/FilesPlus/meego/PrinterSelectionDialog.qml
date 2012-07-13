import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: printerSelectionDialog
    style: SelectionDialogStyle { dim: 0.9 }

    property string srcFilePath
    
    titleText: "Print " + fsModel.getFileName(srcFilePath) + " to"
//    titleIcon: "FilesPlusIcon.svg"

    onStatusChanged: {
//        console.debug("PrinterSelectionDialog onStatusChanged " + status + " model.count " + model.count);
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
        }
    }

    onAccepted: {
        // Print on selected printer index.
        var pid = gcpClient.getPrinterId(printerSelectionDialog.selectedIndex);
        console.debug("printerSelectionDialog onAccepted pid=" + pid + " srcFilePath=" + srcFilePath);
        if (pid != "") {
            // Open uploadProgressBar for printing.
            uploadProgressDialog.srcFilePath = srcFilePath;
            uploadProgressDialog.titleText = "Printing";
            uploadProgressDialog.autoClose = false;
            uploadProgressDialog.open();
            
            gcpClient.submit(pid, srcFilePath);
        }
    }
    
    onRejected: {
        // Reset popupToolPanel.
        popupToolPanel.selectedFilePath = "";
        popupToolPanel.selectedFileIndex = -1;
    }
}
