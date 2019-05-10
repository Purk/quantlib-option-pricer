QT -= gui
QT += core network

CONFIG += c++1z console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# ..ISYSTEM= fixes bug with Qt5+ and gcc6 with: stdlib.h: no such file or dir.
# ref: https://github.com/voidlinux/void-packages/issues/5254
QMAKE_CFLAGS_ISYSTEM=

SOURCES += main.cpp \
    pricer.cpp

INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include

INCLUDEPATH += /usr/include
DEPENDPATH += /usr/include

LIBS += -L/usr/lib/ -lQuantLib

HEADERS += \
    pricer.h
