import QtQuick 1.1
import com.nokia.meego 1.0

Menu {
    id: menu
    z: 2
    signal closed()

    property variant enabledMenus: []
    property variant disabledMenus: []

    function isEnabled(menuName) {
        if (enabledMenus && enabledMenus.length > 0) {
            return (enabledMenus.indexOf(menuName) != -1);
        }
        return true;
    }

    function isDisabled(menuName) {
        if (disabledMenus && disabledMenus.length > 0) {
            return (disabledMenus.indexOf(menuName) != -1);
        }
        return false;
    }

    function toggleMenuItems() {
//        console.debug("MenuWithIcon toggleMenuItems content[0] " + content[0]);
//        console.debug("MenuWithIcon toggleMenuItems content[0].children.length " + content[0].children.length);
        var actualMenuItemCount = 0;
        for (var i=0; i<content[0].children.length; i++) {
            var menuItem = content[0].children[i];
            if (!isEnabled(menuItem.name) || isDisabled(menuItem.name)) {
//                console.debug("MenuWithIcon toggleMenuItems menuItem i " + i + " " + menuItem.toString() + " " + menuItem.name + " is hidden.");
                menuItem.visible = false;
            } else {
//                console.debug("MenuWithIcon toggleMenuItems menuItem i " + i + " " + menuItem.toString() + " " + menuItem.name + " is shown.");
                menuItem.visible = isMenuItemVisible(menuItem);
            }
            if (menuItem.visible) actualMenuItemCount++;
        }
    }

    // Override this function with menuItem logic.
    function isMenuItemVisible(menuItem) {
        return true;
    }

    function updateBgImageSources() {
        var childrenStartIndex = 0;
        var childrenEndIndex = content[0].children.length - 1;

        for (var i=0; i<content[0].children.length; i++) {
            var menuItem = content[0].children[i];
            if (menuItem.visible) {
                childrenStartIndex = i;
                break;
            }
        }
        for (var i=content[0].children.length-1; i>=0; i--) {
            var menuItem = content[0].children[i];
            if (menuItem.visible) {
                childrenEndIndex = i;
                break;
            }
        }

        var childrenCount = childrenEndIndex - childrenStartIndex + 1;

//        console.debug("MenuWithIcon updateBgImageSources childrenStartIndex " + childrenStartIndex + " childrenEndIndex " + childrenEndIndex + " childrenCount " + childrenCount);
        for (var i=0; i<content[0].children.length; i++) {
            var menuItem = content[0].children[i];
            if (menuItem.visible) {
                menuItem.updateBgImageSource(childrenStartIndex, childrenEndIndex, childrenCount);
            }
        }
    }

    function prepareMenuItems() {
        toggleMenuItems();
        updateBgImageSources();
    }

    onContentChanged: {
//        console.debug("MenuWithIcon onContentChanged");
        toggleMenuItems();
        updateBgImageSources();
    }

    onStatusChanged: {
//        console.debug("MenuWithIcon onStatusChanged status " + status);
        if (status == DialogStatus.Opening) {
            toggleMenuItems();
            updateBgImageSources();
//        } else if (status == DialogStatus.Open) {
//            console.debug("MenuWithIcon onStatusChanged height " + height + " implicitHeight " + implicitHeight);
        } else if (status == DialogStatus.Closed) {
            closed();
        }
    }
}
