cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(olive-editor LANGUAGES CXX)

option(BUILD_DOXYGEN "Build Doxygen documentation" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(OLIVE_DEFINITIONS -DQT_DEPRECATED_WARNINGS)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(UNIX AND NOT APPLE AND NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()
find_package(OpenGL REQUIRED)

find_package(Qt5 5.7 REQUIRED
  COMPONENTS
  Core
  Gui
  Widgets
  Multimedia
  OpenGL
  Svg
  LinguistTools
)

find_package(FFMPEG 3.4 REQUIRED
  COMPONENTS
  avutil
  avcodec
  avformat
  avfilter
  swscale
  swresample
)

find_package(frei0r)
if(NOT FREI0R_FOUND)
  list(APPEND OLIVE_DEFINITIONS -DNOFREI0R)
endif()

if(WIN32)
  find_package(OpenColorIO)
  if(OPENCOLORIO_FOUND)
    list(APPEND OLIVE_DEFINITIONS -DOLIVE_OCIO)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
elseif(UNIX AND NOT APPLE)
  # Fallback for Ubuntu/Launchpad (extracts Git hash from debian/changelog rather than Git repo)
  # (see https://answers.launchpad.net/launchpad/+question/678556)
  execute_process(COMMAND sh -c "grep -Po '(?<=-)(([a-z0-9])\\w+)(?=\\+)' -m 1 changelog"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/debian
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
if(DEFINED GIT_HASH)
  message("Olive: git hash = " "${GIT_HASH}")
  list(APPEND OLIVE_DEFINITIONS -DGITHASH="${GIT_HASH}")
else()
  message("Olive: No git hash defined!")
endif()

find_package(Doxygen)

set(OLIVE_SOURCES
  dialogs/aboutdialog.cpp
  dialogs/aboutdialog.h
  dialogs/actionsearch.cpp
  dialogs/actionsearch.h
  dialogs/advancedvideodialog.cpp
  dialogs/advancedvideodialog.h
  dialogs/autocutsilencedialog.cpp
  dialogs/autocutsilencedialog.h
  dialogs/clippropertiesdialog.cpp
  dialogs/clippropertiesdialog.h
  dialogs/debugdialog.cpp
  dialogs/debugdialog.h
  dialogs/demonotice.cpp
  dialogs/demonotice.h
  dialogs/exportdialog.cpp
  dialogs/exportdialog.h
  dialogs/loaddialog.cpp
  dialogs/loaddialog.h
  dialogs/mediapropertiesdialog.cpp
  dialogs/mediapropertiesdialog.h
  dialogs/newsequencedialog.cpp
  dialogs/newsequencedialog.h
  dialogs/preferencesdialog.cpp
  dialogs/preferencesdialog.h
  dialogs/proxydialog.cpp
  dialogs/proxydialog.h
  dialogs/replaceclipmediadialog.cpp
  dialogs/replaceclipmediadialog.h
  dialogs/speeddialog.cpp
  dialogs/speeddialog.h
  dialogs/texteditdialog.cpp
  dialogs/texteditdialog.h
  effects/fields/boolfield.cpp
  effects/fields/boolfield.h
  effects/fields/buttonfield.cpp
  effects/fields/buttonfield.h
  effects/fields/colorfield.cpp
  effects/fields/colorfield.h
  effects/fields/combofield.cpp
  effects/fields/combofield.h
  effects/fields/doublefield.cpp
  effects/fields/doublefield.h
  effects/fields/filefield.cpp
  effects/fields/filefield.h
  effects/fields/fontfield.cpp
  effects/fields/fontfield.h
  effects/fields/labelfield.cpp
  effects/fields/labelfield.h
  effects/fields/stringfield.cpp
  effects/fields/stringfield.h
  effects/internal/audionoiseeffect.cpp
  effects/internal/audionoiseeffect.h
  effects/internal/blending.frag
  effects/internal/common.vert
  effects/internal/cornerpin.frag
  effects/internal/cornerpin.vert
  effects/internal/cornerpineffect.cpp
  effects/internal/cornerpineffect.h
  effects/internal/crossdissolvetransition.cpp
  effects/internal/crossdissolvetransition.h
  effects/internal/cubetransition.h
  effects/internal/dropshadow.frag
  effects/internal/dropshadoweffect.cpp
  effects/internal/dropshadoweffect.h
  effects/internal/exponentialfadetransition.cpp
  effects/internal/exponentialfadetransition.h
  effects/internal/fillleftrighteffect.cpp
  effects/internal/fillleftrighteffect.h
  effects/internal/frei0reffect.cpp
  effects/internal/frei0reffect.h
  effects/internal/internalshaders.qrc
  effects/internal/linearfadetransition.cpp
  effects/internal/linearfadetransition.h
  effects/internal/logarithmicfadetransition.cpp
  effects/internal/logarithmicfadetransition.h
  effects/internal/ocio.frag
  effects/internal/paneffect.cpp
  effects/internal/paneffect.h
  effects/internal/premultiply.frag
  effects/internal/richtexteffect.cpp
  effects/internal/richtexteffect.h
  effects/internal/shakeeffect.cpp
  effects/internal/shakeeffect.h
  effects/internal/solideffect.cpp
  effects/internal/solideffect.h
  effects/internal/texteffect.cpp
  effects/internal/texteffect.h
  effects/internal/timecodeeffect.cpp
  effects/internal/timecodeeffect.h
  effects/internal/toneeffect.cpp
  effects/internal/toneeffect.h
  effects/internal/transformeffect.cpp
  effects/internal/transformeffect.h
  effects/internal/voideffect.cpp
  effects/internal/voideffect.h
  effects/internal/volumeeffect.cpp
  effects/internal/volumeeffect.h
  effects/internal/vsthost.cpp
  effects/internal/vsthost.h
  effects/effect.cpp
  effects/effect.h
  effects/effectfield.cpp
  effects/effectfield.h
  effects/effectfields.h
  effects/effectgizmo.cpp
  effects/effectgizmo.h
  effects/effectloaders.cpp
  effects/effectloaders.h
  effects/effectrow.cpp
  effects/effectrow.h
  effects/keyframe.cpp
  effects/keyframe.h
  effects/transition.cpp
  effects/transition.h
  global/config.cpp
  global/config.h
  global/debug.cpp
  global/debug.h
  global/global.cpp
  global/global.h
  global/math.cpp
  global/math.h
  global/path.cpp
  global/path.h
  include/vestige.h
  panels/effectcontrols.cpp
  panels/effectcontrols.h
  panels/grapheditor.cpp
  panels/grapheditor.h
  panels/panels.cpp
  panels/panels.h
  panels/project.cpp
  panels/project.h
  panels/timeline.cpp
  panels/timeline.h
  panels/viewer.cpp
  panels/viewer.h
  project/clipboard.cpp
  project/clipboard.h
  project/footage.cpp
  project/footage.h
  project/loadthread.cpp
  project/loadthread.h
  project/media.cpp
  project/media.h
  project/previewgenerator.cpp
  project/previewgenerator.h
  project/projectelements.h
  project/projectfilter.cpp
  project/projectfilter.h
  project/projectmodel.cpp
  project/projectmodel.h
  project/proxygenerator.cpp
  project/proxygenerator.h
  project/sourcescommon.cpp
  project/sourcescommon.h
  rendering/audio.cpp
  rendering/audio.h
  rendering/cacher.cpp
  rendering/cacher.h
  rendering/clipqueue.cpp
  rendering/clipqueue.h
  rendering/exportthread.cpp
  rendering/exportthread.h
  rendering/framebufferobject.cpp
  rendering/framebufferobject.h
  rendering/renderfunctions.cpp
  rendering/renderfunctions.h
  rendering/renderthread.cpp
  rendering/renderthread.h
  timeline/clip.cpp
  timeline/clip.h
  timeline/marker.cpp
  timeline/marker.h
  timeline/mediaimportdata.cpp
  timeline/mediaimportdata.h
  timeline/selection.h
  timeline/sequence.cpp
  timeline/sequence.h
  ui/audiomonitor.cpp
  ui/audiomonitor.h
  ui/blur.cpp
  ui/blur.h
  ui/clickablelabel.cpp
  ui/clickablelabel.h
  ui/collapsiblewidget.cpp
  ui/collapsiblewidget.h
  ui/columnedgridlayout.cpp
  ui/columnedgridlayout.h
  ui/colorbutton.cpp
  ui/colorbutton.h
  ui/comboboxex.cpp
  ui/comboboxex.h
  ui/cursors.cpp
  ui/cursors.h
  ui/effectui.cpp
  ui/effectui.h
  ui/embeddedfilechooser.cpp
  ui/embeddedfilechooser.h
  ui/flowlayout.cpp
  ui/flowlayout.h
  ui/focusfilter.cpp
  ui/focusfilter.h
  ui/fontcombobox.cpp
  ui/fontcombobox.h
  ui/graphview.cpp
  ui/graphview.h
  ui/icons.cpp
  ui/icons.h
  ui/keyframedrawing.cpp
  ui/keyframedrawing.h
  ui/keyframenavigator.cpp
  ui/keyframenavigator.h
  ui/keyframeview.cpp
  ui/keyframeview.h
  ui/labelslider.cpp
  ui/labelslider.h
  ui/mainwindow.cpp
  ui/mainwindow.h
  ui/mediaiconservice.cpp
  ui/mediaiconservice.h
  ui/menu.cpp
  ui/menu.h
  ui/menuhelper.cpp
  ui/menuhelper.h
  ui/panel.cpp
  ui/panel.h
  ui/playbutton.cpp
  ui/playbutton.h
  ui/rectangleselect.cpp
  ui/rectangleselect.h
  ui/resizablescrollbar.cpp
  ui/resizablescrollbar.h
  ui/scrollarea.cpp
  ui/scrollarea.h
  ui/sourceiconview.cpp
  ui/sourceiconview.h
  ui/sourcetable.cpp
  ui/sourcetable.h
  ui/styling.cpp
  ui/styling.h
  ui/texteditex.cpp
  ui/texteditex.h
  ui/timelineheader.cpp
  ui/timelineheader.h
  ui/timelinetools.h
  ui/timelinewidget.cpp
  ui/timelinewidget.h
  ui/updatenotification.cpp
  ui/updatenotification.h
  ui/viewercontainer.cpp
  ui/viewercontainer.h
  ui/viewerwidget.cpp
  ui/viewerwidget.h
  ui/viewerwindow.cpp
  ui/viewerwindow.h
  undo/comboaction.cpp
  undo/comboaction.h
  undo/undo.cpp
  undo/undo.h
  undo/undostack.cpp
  undo/undostack.h
  main.cpp
)

set(OLIVE_RESOURCES
  cursors/cursors.qrc
  effects/internal/internalshaders.qrc
  icons/icons.qrc
)

set(OLIVE_EFFECTS
  effects/shaders/boxblur.frag
  effects/shaders/boxblur.xml
  effects/shaders/bulge.frag
  effects/shaders/bulge.xml
  effects/shaders/chromakey.frag
  effects/shaders/chromakey.xml
  effects/shaders/chromaticaberration.frag
  effects/shaders/chromaticaberration.xml
  effects/shaders/colorcorrection.frag
  effects/shaders/colorcorrection.xml
  effects/shaders/colorsel.frag
  effects/shaders/colorsel.xml
  effects/shaders/common.frag
  effects/shaders/common.vert
  effects/shaders/crop.frag
  effects/shaders/crop.xml
  effects/shaders/crossstitch.frag
  effects/shaders/crossstitch.xml
  effects/shaders/directionalblur.frag
  effects/shaders/directionalblur.xml
  effects/shaders/dropshadow.xml.disabled
  effects/shaders/emboss.frag
  effects/shaders/emboss.xml
  effects/shaders/findedges.frag
  effects/shaders/findedges.xml.disabled
  effects/shaders/fisheye.frag
  effects/shaders/fisheye.xml
  effects/shaders/flip.frag
  effects/shaders/flip.xml
  effects/shaders/gaussianblur.frag
  effects/shaders/gaussianblur.xml
  effects/shaders/huesatbri.frag
  effects/shaders/huesatbri.xml
  effects/shaders/invert.frag
  effects/shaders/invert.xml
  effects/shaders/lumakey.frag
  effects/shaders/lumakey.xml
  effects/shaders/noise.frag
  effects/shaders/noise.xml
  effects/shaders/pixelate.frag
  effects/shaders/pixelate.xml
  effects/shaders/posterize.frag
  effects/shaders/posterize.xml
  effects/shaders/radialblur.frag
  effects/shaders/radialblur.xml
  effects/shaders/ripple.frag
  effects/shaders/ripple.xml
  effects/shaders/sphere.frag
  effects/shaders/sphere.xml
  effects/shaders/swirl.frag
  effects/shaders/swirl.xml
  effects/shaders/tile.frag
  effects/shaders/tile.xml
  effects/shaders/toonify.frag
  effects/shaders/toonify.xml
  effects/shaders/vignette.frag
  effects/shaders/vignette.xml
  effects/shaders/volumetriclight.frag
  effects/shaders/volumetriclight.xml
  effects/shaders/wave.frag
  effects/shaders/wave.xml
)

qt5_add_translation(OLIVE_QM_FILES
  ts/olive_ar.ts
  ts/olive_bs.ts
  ts/olive_cs.ts
  ts/olive_de.ts
  ts/olive_es.ts
  ts/olive_fr.ts
  ts/olive_it.ts
  ts/olive_ru.ts
  ts/olive_sr.ts
  ts/olive_id.ts
  ts/olive_tr.ts
  ts/olive_pt-BR.ts
  ts/olive_uk.ts
  ts/olive_zh-CN.ts
  ts/olive_zh-TW.ts
)

set(OLIVE_TARGET "olive-editor")
if(APPLE)
  set(OLIVE_TARGET "Olive")
endif()

add_executable(${OLIVE_TARGET}
  ${OLIVE_SOURCES}
  ${OLIVE_RESOURCES}
  ${OLIVE_EFFECTS}
  ${OLIVE_QM_FILES}
)

target_compile_definitions(${OLIVE_TARGET} PRIVATE ${OLIVE_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_link_libraries(${OLIVE_TARGET}
  PRIVATE
  OpenGL::GL
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Multimedia
  Qt5::OpenGL
  Qt5::Svg
  FFMPEG::avutil
  FFMPEG::avcodec
  FFMPEG::avformat
  FFMPEG::avfilter
  FFMPEG::swscale
  FFMPEG::swresample
)

if(WIN32 AND OPENCOLORIO_FOUND)
  target_link_libraries(${OLIVE_TARGET} PRIVATE OpenColorIO)
endif()

if(DOXYGEN_FOUND AND BUILD_DOXYGEN)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()

if(UNIX AND NOT APPLE)
  install(TARGETS ${OLIVE_TARGET} RUNTIME DESTINATION bin)
  install(FILES ${OLIVE_EFFECTS} DESTINATION share/olive-editor/effects)
  install(FILES packaging/linux/org.olivevideoeditor.Olive.desktop DESTINATION share/applications)
  install(FILES packaging/linux/org.olivevideoeditor.Olive.appdata.xml DESTINATION share/metainfo)
  install(FILES packaging/linux/org.olivevideoeditor.Olive.xml DESTINATION share/mime/packages)
  install(FILES packaging/linux/icons/16x16/org.olivevideoeditor.Olive.png DESTINATION share/icons/hicolor/16x16/apps)
  install(FILES packaging/linux/icons/32x32/org.olivevideoeditor.Olive.png DESTINATION share/icons/hicolor/32x32/apps)
  install(FILES packaging/linux/icons/48x48/org.olivevideoeditor.Olive.png DESTINATION share/icons/hicolor/48x48/apps)
  install(FILES packaging/linux/icons/64x64/org.olivevideoeditor.Olive.png DESTINATION share/icons/hicolor/64x64/apps)
  install(FILES packaging/linux/icons/128x128/org.olivevideoeditor.Olive.png DESTINATION share/icons/hicolor/128x128/apps)
  install(FILES packaging/linux/icons/256x256/org.olivevideoeditor.Olive.png DESTINATION share/icons/hicolor/256x256/apps)
  install(FILES packaging/linux/icons/512x512/org.olivevideoeditor.Olive.png DESTINATION share/icons/hicolor/512x512/apps)
  install(FILES packaging/linux/icons/16x16/application-vnd.olive-project.png DESTINATION share/icons/hicolor/16x16/mimetypes)
  install(FILES packaging/linux/icons/32x32/application-vnd.olive-project.png DESTINATION share/icons/hicolor/32x32/mimetypes)
  install(FILES packaging/linux/icons/48x48/application-vnd.olive-project.png DESTINATION share/icons/hicolor/48x48/mimetypes)
  install(FILES packaging/linux/icons/64x64/application-vnd.olive-project.png DESTINATION share/icons/hicolor/64x64/mimetypes)
  install(FILES packaging/linux/icons/128x128/application-vnd.olive-project.png DESTINATION share/icons/hicolor/128x128/mimetypes)
  install(FILES packaging/linux/icons/256x256/application-vnd.olive-project.png DESTINATION share/icons/hicolor/256x256/mimetypes)
  install(FILES packaging/linux/icons/512x512/application-vnd.olive-project.png DESTINATION share/icons/hicolor/512x512/mimetypes)
  install(FILES ${OLIVE_QM_FILES} DESTINATION share/olive-editor/ts)
endif()
