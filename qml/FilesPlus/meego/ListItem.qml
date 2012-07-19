import QtQuick 1.1
import "Utility.js" as Utility

Item {
    id: listItem
    width: parent.width
    height: 70
    
    property int mouseX
    property int mouseY
    property bool pressed: false
    property alias showUnderline: underline.visible

    signal postPressed(variant mouse)
    signal postReleased(variant mouse)
    signal clicked(variant mouse)
    signal doubleClicked(variant mouse)
    signal pressAndHold(variant mouse)

    states: [
        State {
            name: "normal"
            when: !pressed
            PropertyChanges {
                target: listItemBG
                source: "image://theme/meegotouch-list-fullwidth-inverted-background"
            }
        },
        State {
            name: "pressed"
            when: pressed
            PropertyChanges {
                target: listItemBG
                source: "image://theme/meegotouch-list-fullwidth-inverted-background-selected"
            }
        }
    ]

    BorderImage {
        id: listItemBG
        anchors.fill : parent
    }

    Rectangle {
        id: underline
        width: parent.width
        height: 1
        color: (theme.inverted) ? "#202020" : "grey"
        anchors.bottom: parent.bottom
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
//            console.debug("ListItem onPressed mouse " + mouse);
            parent.mouseX = mouseX;
            parent.mouseY = mouseY;
            parent.pressed = true;

            var listView = parent.parent.parent;
            listView.currentIndex = index;
//            console.debug(listView + " currentIndex " + listView.currentIndex);

            listItem.postPressed(mouse);
        }
        onReleased: {
//            console.debug("ListItem onReleased mouse " + mouse);
            parent.pressed = false;

            listItem.postReleased(mouse);
        }
        onClicked: {
//            console.debug("ListItem onClicked mouse " + mouse);
            listItem.clicked(mouse);
        }
        onDoubleClicked: {
//            console.debug("ListItem onDoubleClicked mouse " + mouse);
            listItem.doubleClicked(mouse);
        }
        onPressAndHold: {
//            console.debug("ListItem onPressAndHold mouse " + mouse);
            listItem.pressAndHold(mouse);
        }
    }
}
