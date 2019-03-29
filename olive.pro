#-------------------------------------------------
#
# Project created by QtCreator 2018-05-11T10:31:59
#
#-------------------------------------------------

QT       += core gui multimedia opengl svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

mac {
    TARGET = Olive
}
!mac {
    TARGET = olive-editor
}
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

# Tries to get the current Git short hash
system("which git") {
    GITPATH = $$PWD

    win32 {
        GITPATH = $$system(cygpath $$PWD)
    }

    GITHASHVAR = $$system(git --git-dir=\"$$GITPATH/.git\" --work-tree=\"$$GITPATH\" log -1 --format=%h)

    # Fallback for Ubuntu/Launchpad (extracts Git hash from debian/changelog rather than Git repo)
    # (see https://answers.launchpad.net/launchpad/+question/678556)
    isEmpty(GITHASHVAR) {
        GITHASHVAR = $$system(sh $$PWD/debian/gitfromlog.sh $$PWD/debian/changelog)
    }

    DEFINES += GITHASH=\\"\"$$GITHASHVAR\\"\"
}

CONFIG += c++11

SOURCES += \
        main.cpp \
        ui/mainwindow.cpp \
    panels/project.cpp \
    panels/effectcontrols.cpp \
    panels/viewer.cpp \
    panels/timeline.cpp \
    ui/sourcetable.cpp \
    dialogs/aboutdialog.cpp \
    ui/timelinewidget.cpp \
    project/media.cpp \
    project/footage.cpp \
    timeline/sequence.cpp \
    timeline/clip.cpp \
    global/config.cpp \
    dialogs/newsequencedialog.cpp \
    ui/viewerwidget.cpp \
    ui/viewercontainer.cpp \
    dialogs/exportdialog.cpp \
    ui/collapsiblewidget.cpp \
    panels/panels.cpp \
    rendering/exportthread.cpp \
    ui/timelineheader.cpp \
    project/previewgenerator.cpp \
    ui/labelslider.cpp \
    dialogs/preferencesdialog.cpp \
    ui/audiomonitor.cpp \
    undo/undo.cpp \
    ui/scrollarea.cpp \
    ui/comboboxex.cpp \
    ui/colorbutton.cpp \
    dialogs/replaceclipmediadialog.cpp \
    ui/keyframeview.cpp \
    ui/texteditex.cpp \
    dialogs/demonotice.cpp \
    timeline/marker.cpp \
    dialogs/speeddialog.cpp \
    dialogs/mediapropertiesdialog.cpp \
    project/projectmodel.cpp \
    project/loadthread.cpp \
    dialogs/loaddialog.cpp \
    global/debug.cpp \
    global/path.cpp \
    effects/internal/linearfadetransition.cpp \
    effects/internal/transformeffect.cpp \
    effects/internal/solideffect.cpp \
    effects/internal/texteffect.cpp \
    effects/internal/timecodeeffect.cpp \
    effects/internal/audionoiseeffect.cpp \
    effects/internal/paneffect.cpp \
    effects/internal/toneeffect.cpp \
    effects/internal/volumeeffect.cpp \
    effects/internal/crossdissolvetransition.cpp \
    effects/internal/shakeeffect.cpp \
    effects/internal/exponentialfadetransition.cpp \
    effects/internal/logarithmicfadetransition.cpp \
    effects/internal/cornerpineffect.cpp \
    global/math.cpp \
    effects/effect.cpp \
    effects/effectrow.cpp \
    effects/effectgizmo.cpp \
    project/clipboard.cpp \
    ui/resizablescrollbar.cpp \
    ui/sourceiconview.cpp \
    project/sourcescommon.cpp \
    ui/keyframenavigator.cpp \
    panels/grapheditor.cpp \
    ui/graphview.cpp \
    ui/keyframedrawing.cpp \
    ui/clickablelabel.cpp \
    effects/keyframe.cpp \
    ui/rectangleselect.cpp \
    dialogs/actionsearch.cpp \
    ui/embeddedfilechooser.cpp \
    effects/internal/fillleftrighteffect.cpp \
    effects/internal/voideffect.cpp \
    dialogs/texteditdialog.cpp \
    dialogs/debugdialog.cpp \
    ui/viewerwindow.cpp \
    project/projectfilter.cpp \
    effects/effectloaders.cpp \
    effects/internal/vsthost.cpp \
    ui/flowlayout.cpp \
    dialogs/proxydialog.cpp \
    project/proxygenerator.cpp \
    dialogs/advancedvideodialog.cpp \
    ui/cursors.cpp \
    ui/menuhelper.cpp \
    global/global.cpp \
    ui/focusfilter.cpp \
    undo/comboaction.cpp \
    ui/mediaiconservice.cpp \
    ui/panel.cpp \
    effects/internal/dropshadoweffect.cpp \
    rendering/renderfunctions.cpp \
    rendering/renderthread.cpp \
    rendering/cacher.cpp \
    rendering/clipqueue.cpp \
    rendering/audio.cpp \
    dialogs/clippropertiesdialog.cpp \
    rendering/framebufferobject.cpp \
    ui/updatenotification.cpp \
    ui/icons.cpp \
    effects/fields/doublefield.cpp \
    effects/fields/fontfield.cpp \
    effects/effectfield.cpp \
    effects/fields/colorfield.cpp \
    effects/fields/stringfield.cpp \
    effects/fields/boolfield.cpp \
    effects/fields/combofield.cpp \
    effects/fields/filefield.cpp \
    effects/fields/labelfield.cpp \
    effects/fields/buttonfield.cpp \
    ui/effectui.cpp \
    effects/transition.cpp \
    ui/styling.cpp \
    undo/undostack.cpp \
    effects/internal/richtexteffect.cpp \
    ui/blur.cpp \
    ui/menu.cpp \
    timeline/mediaimportdata.cpp \
    dialogs/autocutsilencedialog.cpp \
    ui/columnedgridlayout.cpp \
    rendering/shadergenerators.cpp \
    global/timing.cpp \
    rendering/pixelformats.cpp \
    timeline/timelinefunctions.cpp \
    timeline/track.cpp \
    timeline/tracklist.cpp

HEADERS += \
        ui/mainwindow.h \
    panels/project.h \
    panels/effectcontrols.h \
    panels/viewer.h \
    panels/timeline.h \
    ui/sourcetable.h \
    dialogs/aboutdialog.h \
    ui/timelinewidget.h \
    project/media.h \
    project/footage.h \
    timeline/sequence.h \
    timeline/clip.h \
    global/config.h \
    dialogs/newsequencedialog.h \
    ui/viewerwidget.h \
    ui/viewercontainer.h \
    dialogs/exportdialog.h \
    ui/collapsiblewidget.h \
    panels/panels.h \
    rendering/exportthread.h \
    ui/timelinetools.h \
    ui/timelineheader.h \
    project/previewgenerator.h \
    ui/labelslider.h \
    dialogs/preferencesdialog.h \
    ui/audiomonitor.h \
    undo/undo.h \
    ui/scrollarea.h \
    ui/comboboxex.h \
    ui/colorbutton.h \
    dialogs/replaceclipmediadialog.h \
    ui/keyframeview.h \
    ui/texteditex.h \
    dialogs/demonotice.h \
    timeline/marker.h \
    timeline/selection.h \
    dialogs/speeddialog.h \
    dialogs/mediapropertiesdialog.h \
    project/projectmodel.h \
    project/loadthread.h \
    dialogs/loaddialog.h \
    global/debug.h \
    global/path.h \
    effects/internal/transformeffect.h \
    effects/internal/solideffect.h \
    effects/internal/texteffect.h \
    effects/internal/timecodeeffect.h \
    effects/internal/audionoiseeffect.h \
    effects/internal/paneffect.h \
    effects/internal/toneeffect.h \
    effects/internal/volumeeffect.h \
    effects/internal/shakeeffect.h \
    effects/internal/linearfadetransition.h \
    effects/internal/crossdissolvetransition.h \
    effects/internal/exponentialfadetransition.h \
    effects/internal/logarithmicfadetransition.h \
    effects/internal/cornerpineffect.h \
    global/math.h \
    effects/effect.h \
    effects/effectrow.h \
    effects/internal/cubetransition.h \
    effects/effectgizmo.h \
    project/clipboard.h \
    ui/resizablescrollbar.h \
    ui/sourceiconview.h \
    project/sourcescommon.h \
    ui/keyframenavigator.h \
    panels/grapheditor.h \
    ui/graphview.h \
    ui/keyframedrawing.h \
    ui/clickablelabel.h \
    effects/keyframe.h \
    ui/rectangleselect.h \
    dialogs/actionsearch.h \
    ui/embeddedfilechooser.h \
    effects/internal/fillleftrighteffect.h \
    effects/internal/voideffect.h \
    dialogs/texteditdialog.h \
    dialogs/debugdialog.h \
    ui/viewerwindow.h \
    project/projectfilter.h \
    effects/effectloaders.h \
    effects/internal/vsthost.h \
    ui/flowlayout.h \
    dialogs/proxydialog.h \
    project/proxygenerator.h \
    dialogs/advancedvideodialog.h \
    ui/cursors.h \
    ui/menuhelper.h \
    global/global.h \
    project/projectelements.h \
    ui/focusfilter.h \
    undo/comboaction.h \
    ui/mediaiconservice.h \
    ui/panel.h \
    effects/internal/dropshadoweffect.h \
    rendering/renderfunctions.h \
    rendering/renderthread.h \
    rendering/clipqueue.h \
    rendering/cacher.h \
    rendering/audio.h \
    dialogs/clippropertiesdialog.h \
    rendering/framebufferobject.h \
    ui/updatenotification.h \
    ui/icons.h \
    effects/fields/doublefield.h \
    effects/fields/fontfield.h \
    effects/effectfield.h \
    effects/effectfields.h \
    effects/fields/stringfield.h \
    effects/fields/filefield.h \
    effects/fields/labelfield.h \
    ui/effectui.h \
    effects/fields/boolfield.h \
    effects/fields/buttonfield.h \
    effects/fields/colorfield.h \
    effects/fields/combofield.h \
    effects/transition.h \
    ui/styling.h \
    undo/undostack.h \
    effects/internal/richtexteffect.h \
    ui/blur.h \
    ui/menu.h \
    rendering/qopenglshaderprogramptr.h \
    timeline/mediaimportdata.h \
    dialogs/autocutsilencedialog.h \
    ui/columnedgridlayout.h \
    rendering/shadergenerators.h \
    global/timing.h \
    rendering/pixelformats.h \
    timeline/ghost.h \
    timeline/timelinefunctions.h \
    timeline/track.h \
    timeline/tracklist.h

FORMS +=

TRANSLATIONS += \
    ts/olive_de.ts \
    ts/olive_es.ts \
    ts/olive_fr.ts \
    ts/olive_it.ts \
    ts/olive_cs.ts \
    ts/olive_ar.ts \
    ts/olive_ru.ts \
    ts/olive_uk.ts \
    ts/olive_bs.ts \
    ts/olive_sr.ts \
    ts/olive_id.ts

win32 {
    CONFIG(debug, debug|release) {
        CONFIG += console
    }

    RC_FILE = packaging/windows/resources.rc
    LIBS += -lavutil -lavformat -lavcodec -lavfilter -lswscale -lswresample -lOpenColorIO -lopengl32 -luser32
}

mac {
    LIBS += -L/usr/local/lib -lavutil -lavformat -lavcodec -lavfilter -lswscale -lswresample -lOpenColorIO -framework CoreFoundation
    ICON = packaging/macos/olive.icns
    INCLUDEPATH = /usr/local/include
}

unix:!mac {
    CONFIG += link_pkgconfig
    PKGCONFIG += libavutil libavformat libavcodec libavfilter libswscale libswresample OpenColorIO
}

RESOURCES += \
    icons/icons.qrc \
    effects/internal/internalshaders.qrc \
    cursors/cursors.qrc

unix:!mac:isEmpty(PREFIX) {
    PREFIX = /usr/local
}

unix:!mac:target.path = $$PREFIX/bin

effects.files = $$PWD/effects/shaders/*
unix:!mac:effects.path = $$PREFIX/share/olive-editor/effects

translations.files = $$PWD/ts/*.qm
unix:!mac:translations.path = $$PREFIX/share/olive-editor/ts

unix:!mac {
    metainfo.files = $$PWD/packaging/linux/org.olivevideoeditor.Olive.appdata.xml
    metainfo.path = $$PREFIX/share/metainfo
    desktop.files = $$PWD/packaging/linux/org.olivevideoeditor.Olive.desktop
    desktop.path = $$PREFIX/share/applications
    mime.files = $$PWD/packaging/linux/org.olivevideoeditor.Olive.xml
    mime.path = $$PREFIX/share/mime/packages
    icon16.files = $$PWD/packaging/linux/icons/16x16/org.olivevideoeditor.Olive.png
    icon16.path = $$PREFIX/share/icons/hicolor/16x16/apps
    icon32.files = $$PWD/packaging/linux/icons/32x32/org.olivevideoeditor.Olive.png
    icon32.path = $$PREFIX/share/icons/hicolor/32x32/apps
    icon48.files = $$PWD/packaging/linux/icons/48x48/org.olivevideoeditor.Olive.png
    icon48.path = $$PREFIX/share/icons/hicolor/48x48/apps
    icon64.files = $$PWD/packaging/linux/icons/64x64/org.olivevideoeditor.Olive.png
    icon64.path = $$PREFIX/share/icons/hicolor/64x64/apps
    icon128.files = $$PWD/packaging/linux/icons/128x128/org.olivevideoeditor.Olive.png
    icon128.path = $$PREFIX/share/icons/hicolor/128x128/apps
    icon256.files = $$PWD/packaging/linux/icons/256x256/org.olivevideoeditor.Olive.png
    icon256.path = $$PREFIX/share/icons/hicolor/256x256/apps
    icon512.files = $$PWD/packaging/linux/icons/512x512/org.olivevideoeditor.Olive.png
    icon512.path = $$PREFIX/share/icons/hicolor/512x512/apps
    INSTALLS += target effects translations metainfo desktop mime icon16 icon32 icon48 icon64 icon128 icon256 icon512
}
