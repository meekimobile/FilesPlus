import QtQuick 1.1

// Clipboard element = { "action": "cut", "sourcePath": sourcePath }

ListModel {
    id: clipboard

    function getModelIndex(sourcePath) {
        for (var i=0; i<clipboard.count; i++) {
            if (clipboard.get(i).sourcePath == sourcePath) return i;
        }
        return -1;
    }

    function getActionIcon(index) {
        if (index > -1) {
            if (clipboard.get(index).action == "copy") {
                return "copy.svg";
            } else if (clipboard.get(index).action == "cut") {
                return "trim.svg";
            }
        }
        return "";
    }
}
