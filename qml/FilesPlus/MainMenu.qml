import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: mainMenu
    z: 2

    property variant disabledMenus: []
        
    signal resetCache();

    content: MenuLayout {
        MenuItem {
            text: "Sort by"
            platformSubItemIndicator: true
            onClicked: {
                sortByMenu.open();
            }
        }

        MenuItem {
            text: "Reset CloudPrint"
            onClicked: {
                if (gcpClient) gcpClient.refreshAccessToken();
            }
        }

        MenuItem {
            text: "About"
            onClicked: {
                pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
            }
        }
        
        MenuItem {
            text: "Help"
            onClicked: {
                Qt.openUrlExternally("http://sites.google.com/site/meekimobile");
            }
        }
        
        MenuItem {
            id: menuResetCache
            text: "Reset Cache"
            visible: (disabledMenus.indexOf(menuResetCache.text) != -1)
            onClicked: {
                console.debug(disabledMenus.indexOf(menuResetCache.text));
                resetCache();
            }
        }
        
        MenuItem {
            text: "More Apps"
            onClicked: {
                pageStack.push(Qt.resolvedUrl("MoreApps.qml"));
            }
        }
        
        MenuItem {
            text: "Exit"
            onClicked: {
                Qt.quit();
            }
        }
    }
}
