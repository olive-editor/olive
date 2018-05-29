#-------------------------------------------------
#
# Project created by QtCreator 2018-05-11T10:31:59
#
#-------------------------------------------------

QT       += core gui multimedia opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = olive-qt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    panels/project.cpp \
    panels/effectcontrols.cpp \
    panels/viewer.cpp \
    panels/timeline.cpp \
    ui/sourcetable.cpp \
    dialogs/aboutdialog.cpp \
    ui/timelinewidget.cpp \
    io/media.cpp \
    project/sequence.cpp \
    project/clip.cpp \
    playback/playback.cpp \
    playback/audio.cpp \
    io/config.cpp \
    dialogs/newsequencedialog.cpp \
    ui/viewerwidget.cpp \
    ui/viewercontainer.cpp \
    dialogs/exportdialog.cpp \
    ui/collapsiblewidget.cpp \
    effects/transformeffect.cpp \
    panels/panels.cpp \
    effects/volumeeffect.cpp \
    effects/paneffect.cpp \
    project/effect.cpp \
    effects/effects.cpp \
    playback/cacher.cpp \
    io/exportthread.cpp

HEADERS += \
        mainwindow.h \
    panels/project.h \
    panels/effectcontrols.h \
    panels/viewer.h \
    panels/timeline.h \
    ui/sourcetable.h \
    dialogs/aboutdialog.h \
    ui/timelinewidget.h \
    io/media.h \
    project/sequence.h \
    project/clip.h \
    playback/playback.h \
    playback/audio.h \
    effects/effects.h \
    io/config.h \
    ui/timeline-tools.h \
    dialogs/newsequencedialog.h \
    ui/viewerwidget.h \
    ui/viewercontainer.h \
    dialogs/exportdialog.h \
    ui/collapsiblewidget.h \
    project/effect.h \
    panels/panels.h \
    playback/cacher.h \
    io/exportthread.h

FORMS += \
        mainwindow.ui \
    panels/project.ui \
    panels/effectcontrols.ui \
    panels/viewer.ui \
    panels/timeline.ui \
    dialogs/aboutdialog.ui \
    dialogs/newsequencedialog.ui \
    dialogs/exportdialog.ui

win32 {
    LIBS += -L../ffmpeg/lib -lavutil -lavformat -lavcodec -lswscale -lswresample opengl32.lib
    INCLUDEPATH = ../ffmpeg/include
    RC_FILE = icons/win.rc
}

linux {
    LIBS += -lavutil -lavformat -lavcodec -lswscale -lswresample
}

RESOURCES += \
    icons/icons.qrc \
    styles/styles.qrc
