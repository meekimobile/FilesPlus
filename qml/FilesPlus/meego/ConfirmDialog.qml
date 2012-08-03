import QtQuick 1.1
import com.nokia.meego 1.0

CommonDialog {
    id: confirmDialog
    z: 2
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
    
    signal confirm()
    signal reject()

    onButtonClicked: {
        if (index == 0) {
            confirm();
            accept();
        } else {
            reject();
        }
    }
}
