# Add network, script
QT += network script

# Add more folders to ship with the application, here
#folder_01.source = qml/FilesPlus
#folder_01.target = qml
#DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# Change UID3 to protected UID for publishing to Nokia Store.
symbian:TARGET.UID3 = 0xE11DCC9D
#symbian:TARGET.UID3 = 0x20064E45

VERSION = 1.0.1

# Smart Installer package's UID
# This UID is from the protected range and therefore the package will
# fail to install if self-signed. By default qmake uses the unprotected
# range value if unprotected UID is defined for the application and
# 0x2002CCCF value if protected UID is given to the application
symbian:DEPLOYMENT.installer_header = 0x2002CCCF

# Allow network access on Symbian
symbian:TARGET.CAPABILITY += NetworkServices ReadUserData

# Set heap size. min 8M max 32M.
#symbian:TARGET.EPOCHEAPSIZE = 0x800000 0x2000000
# Set heap size. min 8M max 64M.
symbian:TARGET.EPOCHEAPSIZE = 0x800000 0x4000000
#symbian:TARGET.EPOCSTACKSIZE  0x14000

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
CONFIG += mobility
MOBILITY = systeminfo contacts

# Speed up launching on MeeGo/Harmattan when using applauncherd daemon
CONFIG += qdeclarative-boostable

# Add dependency to Symbian components
CONFIG += qt-components

# Added by Nokia Publisher Support's comments.
# In case your SDK does not generate the component dependency from your project file.
symbian {
    my_deployment.pkg_prerules += \
        "; Dependency to Symbian Qt Quick components" \
        "(0x200346DE), 1, 1, 0, {\"Qt Quick components\"}"

    my_deployment.pkg_prerules += \
        "; Vendor names" \
        "%{\"Vendor\"}" \
        ":\"MeekiMobile\""

    DEPLOYMENT += my_deployment
}

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    piechart.cpp \
    foldersizeitem.cpp \
    foldersizeitemlistmodel.cpp \
    pieslice.cpp \
    systeminfohelper.cpp \
    label.cpp \
    tokenpair.cpp \
    gcpclient.cpp \
    dropboxclient.cpp \
    clouddrivemodel.cpp \
    clouddriveitem.cpp \
    gcdclient.cpp \
    clouddrivejob.cpp \
    qnetworkreplywrapper.cpp \
    localfileimageprovider.cpp \
    clouddrivemodelthread.cpp \
    foldersizemodelthread.cpp \
    monitoring.cpp \
    foldersizejob.cpp \
    appinfo.cpp

# Please do not modify the following two lines. Required for deployment.
include(qmlapplicationviewer/qmlapplicationviewer.pri)
qtcAddDeployment()

HEADERS += \
    piechart.h \
    foldersizeitem.h \
    foldersizeitemlistmodel.h \
    pieslice.h \
    systeminfohelper.h \
    label.h \
    tokenpair.h \
    gcpclient.h \
    dropboxclient.h \
    clouddrivemodel.h \
    clouddriveitem.h \
    gcdclient.h \
    clouddrivejob.h \
    qnetworkreplywrapper.h \
    localfileimageprovider.h \
    clouddrivemodelthread.h \
    foldersizemodelthread.h \
    monitoring.h \
    foldersizejob.h \
    appinfo.h

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog \
    qml/FilesPlus/meego/MenuItemWithIcon.qml \
    qml/FilesPlus/meego/CommonDialog.qml \
    qml/FilesPlus/meego/MenuWithIcon.qml \
    qml/FilesPlus/meego/ListItem.qml \
    qml/FilesPlus/meego/PrinterSelectionDialog.qml

OTHER_FILES += \
    qml/FilesPlus/*.svg \
    qml/FilesPlus/*.png \
    qml/FilesPlus/*.js \
    qml/FilesPlus/meego/*.qml \
    qml/FilesPlus/symbian/*.qml

simulator {
    RESOURCES += FilesPlus_symbian.qrc
}
symbian {
    RESOURCES += FilesPlus_symbian.qrc
}
contains(MEEGO_EDITION, harmattan) {
    RESOURCES += FilesPlus_meego.qrc
    DEFINES += Q_WS_HARMATTAN
}

