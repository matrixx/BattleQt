TEMPLATE = lib

QT += network

HEADERS += \
    src/include/connectionmanager.h \
    src/connectionmanager_p.h \
    src/server.h \
    src/client.h

SOURCES += src/connectionmanager.cpp \
    src/server.cpp \
    src/client.cpp

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog

contains(MEEGO_EDITION,harmattan) {
    headers.files = src/include/connectionmanager.h
    headers.path = /usr/include/battleqt/
    target.path = /usr/lib/battleqt/
    INSTALLS += target
}
