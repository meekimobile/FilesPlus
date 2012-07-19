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
                return (theme.inverted) ? "copy.svg" : "copy_inverted.svg";
            } else if (clipboard.get(index).action == "cut") {
                return (theme.inverted) ? "trim.svg" : "trim_inverted.svg";
            }
        }
        return "";
    }

    function clearDeleteActions() {
        var i = 0;
        while (i<clipboard.count) {
            if (clipboard.get(i).action == "delete") {
                // Remove current entry will make next entry takeover current index. It needs not increase index.
                clipboard.remove(i);
            } else {
                i++;
            }
        }
    }

    function addItem(jsobj) {
        var i = getModelIndex(jsobj.sourcePath);
        if (i > -1) {
            clipboard.set(i, jsobj);
            console.debug("clipboard.addItem found and replace index " + i + " action " + jsobj.action + " sourcePath " + jsobj.sourcePath);
        } else {
            clipboard.append(jsobj);
        }
    }
}
