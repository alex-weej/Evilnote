#-------------------------------------------------
#
# Project created by QtCreator 2012-08-21T01:25:09
#
#-------------------------------------------------

QT       += core widgets multimedia opengl macextras

TARGET = Evilnote
TEMPLATE = app

INCLUDEPATH += /Users/alex/Downloads/vstsdk2.4

LIBS += -framework CoreFoundation -framework Carbon -framework Cocoa -framework CoreMIDI

SOURCES += main.cpp \
    en.cpp \
    nodecontextmenuhelper.cpp \
    node.cpp \
    host.cpp \
    nodegraph.cpp \
    mainwindow.cpp \
    nodecreationdialog.cpp \
    vstnode.cpp \
    core.cpp \
    nodegroup.cpp \
    nodewindow.cpp

OBJECTIVE_SOURCES += vsteditorwidget.mm \
    mididevice.mm

HEADERS  += \
    vstnode.h \
    en.h \
    node.h \
    vstmodule.h \
    __headertemplate.h \
    core.h \
    nodegroup.h \
    mixernode.h \
    nodecontextmenuhelper.h \
    vstinfo.h \
    vsteditorwidget.h \
    host.h \
    hostthread.h \
    nodewindow.h \
    nodegraph.h \
    mainwindow.h \
    nodecreationdialog.h \
    mididevice.h

FORMS    += \
    nodewindow.ui
