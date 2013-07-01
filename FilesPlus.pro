# Add modules
QT += declarative network script sql xml
contains(MEEGO_EDITION, harmattan) {
    QT += opengl
}

# Add more folders to ship with the application, here
i18n_folder.source = i18n/*.qm i18n/*.ts
i18n_folder.target = i18n
config_folder.source = config/mime.types
config_folder.target = config
DEPLOYMENTFOLDERS = i18n_folder config_folder

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# Change UID3 to protected UID for publishing to Nokia Store.
#symbian:TARGET.UID3 = 0xE11DCC9D
symbian:TARGET.UID3 = 0x20064E45

VERSION = 1.3.0

# Smart Installer package's UID
# This UID is from the protected range and therefore the package will
# fail to install if self-signed. By default qmake uses the unprotected
# range value if unprotected UID is defined for the application and
# 0x2002CCCF value if protected UID is given to the application
symbian:DEPLOYMENT.installer_header = 0x2002CCCF

# Allow network access on Symbian
symbian:TARGET.CAPABILITY += NetworkServices ReadUserData WriteUserData LocalServices Location ReadDeviceData WriteDeviceData
# Set heap size. min 8M max 32M.
#symbian:TARGET.EPOCHEAPSIZE = 0x800000 0x2000000
# Set heap size. min 8M max 64M.
symbian:TARGET.EPOCHEAPSIZE = 0x800000 0x4000000
symbian:TARGET.EPOCSTACKSIZE = 0x100000 # 1M
#symbian:TARGET.EPOCSTACKSIZE = 0x14000 # 80K

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
CONFIG += mobility
MOBILITY = systeminfo connectivity
symbian {
    MOBILITY += contacts messaging
}

# Speed up launching on MeeGo/Harmattan when using applauncherd daemon
CONFIG += qdeclarative-boostable

# Add dependency to Symbian components
CONFIG += qt-components

#share-ui
contains(MEEGO_EDITION, harmattan) {
    CONFIG += shareuiinterface-maemo-meegotouch share-ui-plugin share-ui-common mdatauri
}

# Added by Nokia Publisher Support's comments.
# In case your SDK does not generate the component dependency from your project file.
symbian {
    my_deployment.pkg_prerules += \
        "; Dependency to Symbian Qt Quick components" \
        "(0x200346DE), 1, 1, 0, {\"Qt Quick components\"}"

    my_deployment.pkg_prerules += \
        "; Vendor names" \
        "%{\"MeekiMobile\"}" \
        ":\"MeekiMobile\""

    DEPLOYMENT += my_deployment

    MMP_RULES += "LIBRARY efsrv.lib"
}

# Add file compression library.
symbian {
    # Add QuaZip library.
    INCLUDEPATH += ../quazip-0.5.1/quazip
    LIBS += -lquazip.lib

    # Add UnRar library.
#    DEFINES += SYMBIAN
#    INCLUDEPATH += ../unrar
#    LIBS += -lunrar.lib
}

contains(MEEGO_EDITION, harmattan) {
    # Add QuaZip library.
    INCLUDEPATH += . ../quazip-0.5.1/quazip
    LIBS += -L../quazip-build-harmattan/quazip -lquazip

    # Add UnRar library.
    INCLUDEPATH += . ../unrar
    LIBS += -L../unrar-build-harmattan -lunrar
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
    remoteimageprovider.cpp \
    clouddrivemodelthread.cpp \
    foldersizemodelthread.cpp \
    monitoring.cpp \
    foldersizejob.cpp \
    appinfo.cpp \
    clipboardmodel.cpp \
    customqnetworkaccessmanagerfactory.cpp \
    customqnetworkaccessmanager.cpp \
    ftpclient.cpp \
    skydriveclient.cpp \
    sleeper.cpp \
    qftpwrapper.cpp \
    clouddriveclient.cpp \
    webdavclient.cpp \
    cacheimageworker.cpp \
    contenttypehelper.cpp \
    bookmarksmodel.cpp \
    clouddrivemodelitem.cpp \
    compressedfoldermodel.cpp \
    compressedfoldermodelworker.cpp \
    compressedfoldermodelitem.cpp \
    boxclient.cpp

symbian {
SOURCES += \
    bluetoothclient.cpp \
    messageclient.cpp
}

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
    remoteimageprovider.h \
    clouddrivemodelthread.h \
    foldersizemodelthread.h \
    monitoring.h \
    foldersizejob.h \
    appinfo.h \
    clipboardmodel.h \
    customqnetworkaccessmanagerfactory.h \
    customqnetworkaccessmanager.h \
    ftpclient.h \
    skydriveclient.h \
    sleeper.h \
    qftpwrapper.h \
    clouddriveclient.h \
    webdavclient.h \
    cacheimageworker.h \
    contenttypehelper.h \
    bookmarksmodel.h \
    clouddrivemodelitem.h \
    compressedfoldermodel.h \
    compressedfoldermodelworker.h \
    compressedfoldermodelitem.h \
    boxclient.h

symbian {
HEADERS += \
    bluetoothclient.h \
    messageclient.h
}

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog

OTHER_FILES += \
    qml/FilesPlus/*.svg \
    qml/FilesPlus/*.png \
    qml/FilesPlus/*.js \
    qml/FilesPlus/meego/*.qml \
    qml/FilesPlus/symbian/*.qml \
    i18n/*.* \
    config/*.*

simulator {
    RESOURCES += FilesPlus_symbian.qrc
    # Define VER to use in cpp.
    VERSTR = '$${VERSION}'  # place quotes around the version string
    DEFINES += VER=\"$${VERSTR}\" # create a VER macro containing the version string
}
symbian {
    RESOURCES += FilesPlus_symbian.qrc
    # Define VER to use in cpp.
    VERSTR = '$${VERSION}'  # place quotes around the version string
    DEFINES += VER=\"$${VERSTR}\" # create a VER macro containing the version string
}
contains(MEEGO_EDITION, harmattan) {
    message( MEEGO_EDITION $${MEEGO_EDITION} $${MEEGO_VERSION_MAJOR} $${MEEGO_VERSION_MINOR} $${MEEGO_VERSION_PATCH} )

    RESOURCES += FilesPlus_meego.qrc
    DEFINES += Q_WS_HARMATTAN

    # Define VER to use in cpp.
    VERSTR = '\\"$${VERSION}\\"'  # place quotes around the version string
    DEFINES += VER=\"$${VERSTR}\" # create a VER macro containing the version string
}

TRANSLATIONS += \
    i18n/FilesPlus_en.ts \
    i18n/FilesPlus_ru.ts \
    i18n/FilesPlus_ru_2.ts \
    i18n/FilesPlus_zh.ts \
    i18n/FilesPlus_de.ts \
    i18n/FilesPlus_it.ts

message( VERSION $${VERSION} )
message( VERSTR $${VERSTR} )
