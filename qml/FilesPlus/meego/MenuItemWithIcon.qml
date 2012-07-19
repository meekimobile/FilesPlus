import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

MenuItem {
    id: root

    property alias platformLeftMargin: menuItemStyle.leftMargin
    property bool platformSubItemIndicator: false
    property string iconSource
    property bool checked

    platformStyle: MenuItemStyle {
        id: menuItemStyle
        leftMargin: paddingMedium * 2 + (iconSource == "" ? 0 : menuIcon.width + paddingMedium)

        property real paddingMedium: 10
    }

    function findBgImageSource(start, end, count) {
//        console.debug("menuItemWithIcon findBgImageSource start " + start + " end " + end + " count " + count);
//        console.debug("menuItemWithIcon findBgImageSource theme.inverted " + theme.inverted);

        var invertedText = (theme.inverted) ? "-inverted" : "";

        // Get BG image.
        if (count == 1) {
            return (pressed ? "image://theme/meegotouch-list" + invertedText + "-background-pressed" : "image://theme/meegotouch-list" + invertedText + "-background");
        } else if (root.parent.children[start] == root) {
            return (pressed ? "image://theme/meegotouch-list" + invertedText + "-background-pressed-vertical-top" : "image://theme/meegotouch-list" + invertedText + "-background-vertical-top");
        } else if (root.parent.children[end] == root) {
            return (pressed ? "image://theme/meegotouch-list" + invertedText + "-background-pressed-vertical-bottom" : "image://theme/meegotouch-list" + invertedText + "-background-vertical-bottom");
        } else {
            return (pressed ? "image://theme/meegotouch-list" + invertedText + "-background-pressed-vertical-center" : "image://theme/meegotouch-list" + invertedText + "-background-vertical-center");
        }
    }

    function updateBgImageSource(start, end, count) {
        var bgImageSource = findBgImageSource(start, end, count);
        setBgImageSource(bgImageSource);
    }

    function getBgImageSource() {
        // Find backgroundImage.
        for (var i=0; i<root.children.length; i++) {
//            console.debug("root.children[i].toString() " + root.children[i].toString());
            if (root.children[i].toString().indexOf("BorderImage")) {
                break;
            }
        }

        return root.children[i].source;
    }

    function setBgImageSource(imageSource) {
        // Update backgroundImage.
        for (var i=0; i<root.children.length; i++) {
//            console.debug("root.children[i].toString() " + root.children[i].toString());
            if (root.children[i].toString().indexOf("BorderImage")) {
                break;
            }
        }

        root.children[i].source = imageSource;
    }

    onPressedChanged: {
        if (pressed) {
            var src = getBgImageSource() + "";
            setBgImageSource(Utility.replace(src,"-background","-background-pressed"));
        } else {
            var src = getBgImageSource() + "";
            setBgImageSource(Utility.replace(src,"-background-pressed", "-background"));
        }
    }

    Component.onCompleted: {
        // Issue: Bundled MenuItem doesn't support extension. It will shows incorrect backgroundImage.
        updateBgImageSource(0, root.parent.children.length - 1, root.parent.children.length);
    }


    Image {
        id: menuIcon

        anchors {
            left: parent.left
            leftMargin: platformStyle.paddingMedium * 2
            verticalCenter: parent.verticalCenter
        }

        source: (checked ? (theme.inverted ? "image://theme/icon-m-toolbar-done-white" : "image://theme/icon-m-toolbar-done") : iconSource)
        visible: source != undefined
        sourceSize.width: platformStyle.graphicSizeSmall
        sourceSize.height: platformStyle.graphicSizeSmall
    }

    Image {
        id: subItemIcon

        anchors {
            right: parent.right
            rightMargin: platformStyle.paddingMedium
            verticalCenter: parent.verticalCenter
        }

        source: "image://theme/icon-m-common-drilldown-arrow" + (theme.inverted ? "-inverse" : "")
        visible: platformSubItemIndicator
        sourceSize.width: platformStyle.graphicSizeSmall
        sourceSize.height: platformStyle.graphicSizeSmall
    }
}
