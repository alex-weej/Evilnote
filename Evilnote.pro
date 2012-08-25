#-------------------------------------------------
#
# Project created by QtCreator 2012-08-21T01:25:09
#
#-------------------------------------------------

QT       += core gui multimedia

TARGET = Evilnote
TEMPLATE = app

INCLUDEPATH += /Users/alex/Downloads/vstsdk2.4

LIBS += -framework CoreFoundation -framework Carbon -framework Cocoa

SOURCES += main.cpp\
        mainwindow.cpp \
    cocoastuff.mm

OBJECTIVE_SOURCES +=

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

#OTHER_FILES += \
#    carbonshit.mm
