# Add network, script
QT += network script

# Add more folders to ship with the application, here
folder_01.source = qml/FilesPlus
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# Change UID3 to protected UID for publishing to Nokia Store.
#symbian:TARGET.UID3 = 0xE11DCC9D
symbian:TARGET.UID3 = 0x20064E45

VERSION = 1.0.0

# Smart Installer package's UID
# This UID is from the protected range and therefore the package will
# fail to install if self-signed. By default qmake uses the unprotected
# range value if unprotected UID is defined for the application and
# 0x2002CCCF value if protected UID is given to the application
symbian:DEPLOYMENT.installer_header = 0x2002CCCF

# Allow network access on Symbian
symbian:TARGET.CAPABILITY += NetworkServices Location SwEvent

# Set heap size. min 8M max 32M.
#symbian:TARGET.EPOCHEAPSIZE = 0x800000 0x2000000

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
CONFIG += mobility
MOBILITY = systeminfo

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
    foldersizemodel.cpp \
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
    clouddrivemodelthread.cpp

# Please do not modify the following two lines. Required for deployment.
include(qmlapplicationviewer/qmlapplicationviewer.pri)
qtcAddDeployment()

HEADERS += \
    piechart.h \
    foldersizemodel.h \
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
    clouddrivemodelthread.h

OTHER_FILES +=

RESOURCES += \
    FilesPlus.qrc

