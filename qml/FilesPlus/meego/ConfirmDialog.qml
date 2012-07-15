import QtQuick 1.1
import com.nokia.meego 1.0

CommonDialog {
    id: confirmDialog
    z: 2
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [qsTr("OK"), qsTr("Cancel")]
    
    signal confirm()

    onButtonClicked: {
        if (index == 0) {
            confirm();
            accept();
        } else {
            reject();
        }
    }
}
