import QtQuick 1.1

MouseArea {
    id: scrollBar

    states: [
        State {
            name: "hide"
            when: !pressed
            PropertyChanges {
                target: background
                opacity: 0
            }
            PropertyChanges {
                target: currentPos
                opacity: 0
            }
        },
        State {
            name: "show"
            when: pressed
            PropertyChanges {
                target: background
                opacity: 0.3
            }
            PropertyChanges {
                target: currentPos
                opacity: 0.7
            }
        }
    ]

    // The properties that define the scrollbar's state.
    // position and pageSize are in the range 0.0 - 1.0.  They are relative to the
    // height of the page, i.e. a pageSize of 0.5 means that you can see 50%
    // of the height of the view.
    // orientation can be either Qt.Vertical or Qt.Horizontal
    property real position
    property real pageSize
    property variant orientation : Qt.Vertical

    // A light, semi-transparent background
    Rectangle {
        id: background
        anchors.fill: parent
        color: "white"
        opacity: 0.3
    }

    // Size the bar to the required size, depending upon the orientation.
    Rectangle {
        id: currentPos
        x: orientation == Qt.Vertical ? 1 : (scrollBar.position * (scrollBar.width-2) + 1)
        y: orientation == Qt.Vertical ? (scrollBar.position * (scrollBar.height-2) + 1) : 1
        width: orientation == Qt.Vertical ? (parent.width-2) : (scrollBar.pageSize * (scrollBar.width-2))
        height: orientation == Qt.Vertical ? (scrollBar.pageSize * (scrollBar.height-2)) : (parent.height-2)
        color: "black"
        opacity: 0.7
    }

    onPressed: {
        console.debug("scrollBar onPressed mouse x " + mouse.x + " y " + mouse.y);
    }
    onPositionChanged: {
        console.debug("scrollBar onPositionChanged mouse x " + mouseX + " y " + mouseY);
    }
    onReleased: {
        console.debug("scrollBar onReleased mouse x " + mouse.x + " y " + mouse.y);
    }
}
