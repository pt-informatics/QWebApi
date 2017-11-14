QT += core gui widgets

include(../src/qwebapi.pri)

TARGET = examples
CONFIG -= app_bundle

HEADERS += \
    testclass.h

SOURCES += \
    main.cpp \
    testclass.cpp

DESTDIR=$$PWD/build
MOC_DIR=$$PWD/.moc
OBJECTS_DIR=$$PWD/.obj
