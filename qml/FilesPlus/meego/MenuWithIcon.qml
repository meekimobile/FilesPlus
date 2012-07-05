import QtQuick 1.1
import com.nokia.meego 1.1

Menu {
    id: menu
    z: 2

    property variant enabledMenus: []
    property variant disabledMenus: []

    signal quit()

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
//        console.debug("MenuWithIcon toggleMenuItems menuLayout.children.length " + menuLayout.children.length);
        for (var i=0; i<menuLayout.children.length; i++) {
            var menuItem = menuLayout.children[i];
            if (!isEnabled(menuItem.text) || isDisabled(menuItem.text)) {
                console.debug("MenuWithIcon toggleMenuItems menuLayout.children i " + i + " " + menuItem.toString() + " " + menuItem.text + " is removed.");
                menuItem.visible = false;
            } else {
                menuItem.visible = true;
            }
        }
    }

    function updateBgImageSources() {
        var childrenStartIndex = 0;
        var childrenEndIndex = menuLayout.children.length - 1;

        for (var i=0; i<menuLayout.children.length; i++) {
            var menuItem = menuLayout.children[i];
            if (menuItem.visible) {
                childrenStartIndex = i;
                break;
            }
        }
        for (var i=menuLayout.children.length-1; i>=0; i--) {
            var menuItem = menuLayout.children[i];
            if (menuItem.visible) {
                childrenEndIndex = i;
                break;
            }
        }

        var childrenCount = childrenEndIndex - childrenStartIndex + 1;

//        console.debug("MenuWithIcon updateBgImageSources childrenStartIndex " + childrenStartIndex + " childrenEndIndex " + childrenEndIndex + " childrenCount " + childrenCount);
        for (var i=0; i<menuLayout.children.length; i++) {
            var menuItem = menuLayout.children[i];
            if (menuItem.visible) {
                menuItem.updateBgImageSource(childrenStartIndex, childrenEndIndex, childrenCount);
            }
        }
    }

    onContentChanged: {
        toggleMenuItems();
        updateBgImageSources();
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            toggleMenuItems();
            updateBgImageSources();
        }
    }
}
