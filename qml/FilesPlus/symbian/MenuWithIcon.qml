import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: menu
    z: 2
    platformInverted: window.platformInverted

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
        for (var i=0; i<content[0].children.length; i++) {
            var menuItem = content[0].children[i];
            if (!isEnabled(menuItem.name) || isDisabled(menuItem.name)) {
//                console.debug("MenuWithIcon toggleMenuItems menuItem i " + i + " " + menuItem.toString() + " " + menuItem.name + " is hidden.");
                menuItem.visible = false;
            } else {
//                console.debug("MenuWithIcon toggleMenuItems menuItem i " + i + " " + menuItem.toString() + " " + menuItem.name + " is shown.");
                menuItem.visible = isMenuItemVisible(menuItem);
            }
        }
    }

    // Override this function with menuItem logic.
    function isMenuItemVisible(menuItem) {
        return true;
    }

    onStatusChanged: {
//        console.debug("MenuWithIcon onStatusChanged status " + status);
        if (status == DialogStatus.Opening) {
            toggleMenuItems();
        } else if (status == DialogStatus.Closed) {
            closed();
        }
    }
}
