#-------------------------------------------------
#
# Project created by QtCreator 2018-05-11T10:31:59
#
#-------------------------------------------------

QT       += core gui multimedia opengl qml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Olive
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
    panels/panels.cpp \
    playback/cacher.cpp \
    io/exportthread.cpp \
    ui/timelineheader.cpp \
    io/previewgenerator.cpp \
    ui/labelslider.cpp \
    dialogs/preferencesdialog.cpp \
    effects/transition.cpp \
    effects/crossdissolvetransition.cpp \
    ui/audiomonitor.cpp \
    project/undo.cpp \
    ui/scrollarea.cpp \
    ui/comboboxex.cpp \
    ui/colorbutton.cpp \
    dialogs/replaceclipmediadialog.cpp \
    ui/fontcombobox.cpp \
    ui/checkboxex.cpp \
    effects/effect.cpp \
    ui/keyframeview.cpp \
    ui/texteditex.cpp \
    dialogs/demonotice.cpp \
    project/marker.cpp \
    dialogs/speeddialog.cpp \
    dialogs/mediapropertiesdialog.cpp \
    io/crc32.cpp \
    dialogs/loaddialog.cpp \
    debug.cpp \
    io/path.cpp \
    effects/qpainterwrapper.cpp \
    effects/internal/linearfadetransition.cpp \
    effects/internal/transformeffect.cpp \
    effects/internal/solideffect.cpp \
    effects/internal/texteffect.cpp \
    effects/internal/audionoiseeffect.cpp \
    effects/internal/paneffect.cpp \
    effects/internal/toneeffect.cpp \
    effects/internal/volumeeffect.cpp

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
    io/config.h \
    dialogs/newsequencedialog.h \
    ui/viewerwidget.h \
    ui/viewercontainer.h \
    dialogs/exportdialog.h \
    ui/collapsiblewidget.h \
    panels/panels.h \
    playback/cacher.h \
    io/exportthread.h \
    ui/timelinetools.h \
    ui/timelineheader.h \
    io/previewgenerator.h \
    ui/labelslider.h \
    dialogs/preferencesdialog.h \
    effects/transition.h \
    ui/audiomonitor.h \
    project/undo.h \
    ui/scrollarea.h \
    ui/comboboxex.h \
    ui/colorbutton.h \
    dialogs/replaceclipmediadialog.h \
    ui/fontcombobox.h \
    ui/checkboxex.h \
    effects/effect.h \
    ui/keyframeview.h \
    ui/texteditex.h \
    dialogs/demonotice.h \
    project/marker.h \
    project/selection.h \
    dialogs/speeddialog.h \
    dialogs/speeddialog.h \
    dialogs/mediapropertiesdialog.h \
    io/crc32.h \
    dialogs/loaddialog.h \
    debug.h \
    io/path.h \
    effects/qpainterwrapper.h \
    effects/internal/transformeffect.h \
    effects/internal/solideffect.h \
    effects/internal/texteffect.h \
    effects/internal/audionoiseeffect.h \
    effects/internal/paneffect.h \
    effects/internal/toneeffect.h \
    effects/internal/volumeeffect.h

FORMS += \
        mainwindow.ui \
    panels/project.ui \
    panels/effectcontrols.ui \
    panels/viewer.ui \
    panels/timeline.ui \
    dialogs/aboutdialog.ui \
    dialogs/newsequencedialog.ui \
    dialogs/exportdialog.ui \
    dialogs/preferencesdialog.ui \
    dialogs/demonotice.ui

win32 {
    RC_FILE = icons/resources.rc
    LIBS += -lavutil -lavformat -lavcodec -lavfilter -lswscale -lswresample -lopengl32
}

mac {
    LIBS += -L/usr/local/lib -lavutil -lavformat -lavcodec -lavfilter -lswscale -lswresample
    ICON = icons/olive.icns
    INCLUDEPATH = /usr/local/include
}

linux {
    LIBS += -lavutil -lavformat -lavcodec -lavfilter -lswscale -lswresample
}

RESOURCES += \
    icons/icons.qrc
