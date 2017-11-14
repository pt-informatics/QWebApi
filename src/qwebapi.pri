QT += network websockets

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/restapi.cpp \
    $$PWD/picohttpparser.c \
    $$PWD/websocketapi.cpp \
    $$PWD/abstractapi.cpp \
    $$PWD/httpserver.cpp

HEADERS += \
    $$PWD/restapi.h \
    $$PWD/picohttpparser.h \
    $$PWD/websocketapi.h \
    $$PWD/abstractapi.h \
    $$PWD/httpserver.h
