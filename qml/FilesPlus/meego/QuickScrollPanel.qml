import QtQuick 1.1

Item {
    id: quickScrollPanel
    anchors.fill: listView
    visible: false

    property bool showScrollBar: listView.moving
    property bool showScrollIndicator: false
    property variant listView
    property int modelIndex: listView.indexAt(listView.contentX, listView.contentY)
    property alias scrollBarWidth: quickScroll.width
    property alias indicatorBarHeight: quickScrollIndicatorBar.height
    property alias indicatorBarTitle: quickScrollIndicatorBarTitle.text
    property int indicatorHeight: height * (height / listView.contentHeight)
    property int maxPosition: height - indicatorHeight
    property int position: listView.contentY * (maxPosition / (listView.contentHeight - height))
    property alias indicatorBarTitleFontPixelSize: quickScrollIndicatorBarTitle.font.pixelSize
    property alias indicatorBarTitleColor: quickScrollIndicatorBarTitle.color
    property alias autoHideInterval: hideTimer.interval
    property bool inverted: false

    function calculateContentY(mouseY) {
        return Math.floor(Math.min(maxPosition, Math.max(0, mouseY)) / maxPosition * (listView.contentHeight - height));
    }

    function calculateModelIndex(mouseY) {
        return Math.floor(Math.min(maxPosition, Math.max(0, mouseY)) / maxPosition * listView.count);
    }

    onShowScrollBarChanged: {
//        console.debug("quickScrollPanel onShowScrollBarChanged " + showScrollBar);
        if (showScrollBar) {
            opacity = 1.0;
            visible = true
            showScrollIndicator = false;
            hideTimer.stop();
        } else {
            hideTimer.restart();
        }
    }

    Rectangle {
        id: quickScrollIndicatorBar
        width: parent.width
        height: 80
        x: 0
        y: Math.min((quickScrollPanel.height - height), Math.max(0, position + (quickScrollIndicator.height / 2) - (height / 2)))
        color: !inverted ? "white" : "black"
        opacity: 0.9
        visible: showScrollIndicator

        Text {
            id: quickScrollIndicatorBarTitle
            width: parent.width - scrollBarWidth - 20
            font.pixelSize: 48
            color: !inverted ? "black" : "white"
            elide: Text.ElideRight
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Rectangle {
        id: quickScroll
        height: parent.height
        width: 80
        anchors.right: parent.right
        color: !inverted ? "white" : "black"
        opacity: 0.3

        Rectangle {
            id: quickScrollIndicator
            width: parent.width - 10
            height: indicatorHeight
            anchors.horizontalCenter: parent.horizontalCenter
            y: position
            color: !inverted ? "white" : "black"
            radius: 10
            opacity: 1.0
        }

        MouseArea {
            anchors.fill: parent
            onPressed: {
//                console.debug("quickScroll onPressed " + mouseX + "," + mouseY);
                hideTimer.stop();
                listView.interactive = false;
                showScrollIndicator = true;
                listView.contentY = calculateContentY(mouseY - (indicatorHeight / 2));
            }
            onPositionChanged: {
//                console.debug("quickScroll onPositionChanged " + mouseX + "," + mouseY);
                listView.contentY = calculateContentY(mouseY - (indicatorHeight / 2));
            }
            onReleased: {
//                console.debug("quickScroll onReleased " + mouseX + "," + mouseY);
                hideTimer.restart();
                listView.interactive = true;
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 3000
        onTriggered: {
            hideAnimation.start();
        }
    }

    SequentialAnimation {
        id: hideAnimation
        NumberAnimation {
            target: quickScrollPanel
            property: "opacity"
            to: 0;
            duration: 500;
        }
        PropertyAction {
            target: quickScrollPanel
            property: "visible"
            value: false
        }
    }
}
