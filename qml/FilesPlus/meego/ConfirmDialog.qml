import QtQuick 1.1
import com.nokia.meego 1.1

CommonDialog {
    id: confirmDialog
    z: 2
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: ["OK", "Cancel"]
    
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
