# Olive - Non-Linear Video Editor
# Copyright (C) 2019 Olive Team
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  main.cpp
)

add_subdirectory(panel)
add_subdirectory(project)
add_subdirectory(render)
add_subdirectory(widget)
add_subdirectory(window)

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

target_compile_options(${OLIVE_TARGET} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-reorder>)

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
  OpenColorIO
)

set(OLIVE_RESOURCES
  cursors/cursors.qrc
  effects/internal/internalshaders.qrc
  icons/icons.qrc
)

set(OLIVE_EFFECTS
  effects/internal/cornerpin.frag
  effects/internal/dropshadow.frag
  effects/internal/ocio.frag
  effects/internal/premultiply.frag
  effects/shaders/boxblur.frag
  effects/shaders/boxblur.xml
  effects/shaders/bulge.frag
  effects/shaders/bulge.xml
  effects/shaders/chromaticaberration.frag
  effects/shaders/chromaticaberration.xml
  effects/shaders/crop.frag
  effects/shaders/crop.xml
  effects/shaders/despill.frag
  effects/shaders/despill.xml
  effects/shaders/directionalblur.frag
  effects/shaders/directionalblur.xml
  effects/shaders/flip.frag
  effects/shaders/flip.xml
  effects/shaders/gaussianblur.frag
  effects/shaders/gaussianblur.xml
  effects/shaders/need\ checking/chromakey.frag
  effects/shaders/need\ checking/chromakey.xml
  effects/shaders/need\ checking/colorcorrection.frag
  effects/shaders/need\ checking/colorcorrection.xml
  effects/shaders/need\ checking/colorsel.frag
  effects/shaders/need\ checking/colorsel.xml
  effects/shaders/need\ checking/common.frag
  effects/shaders/need\ checking/crossstitch.frag
  effects/shaders/need\ checking/crossstitch.xml
  effects/shaders/need\ checking/emboss.frag
  effects/shaders/need\ checking/emboss.xml
  effects/shaders/need\ checking/findedges.frag
  effects/shaders/need\ checking/fisheye.frag
  effects/shaders/need\ checking/fisheye.xml
  effects/shaders/need\ checking/huesatbri.frag
  effects/shaders/need\ checking/huesatbri.xml
  effects/shaders/need\ checking/invert.frag
  effects/shaders/need\ checking/invert.xml
  effects/shaders/need\ checking/lumakey.frag
  effects/shaders/need\ checking/lumakey.xml
  effects/shaders/need\ checking/posterize.frag
  effects/shaders/need\ checking/posterize.xml
  effects/shaders/need\ checking/toonify.frag
  effects/shaders/need\ checking/toonify.xml
  effects/shaders/need\ checking/volumetriclight.frag
  effects/shaders/need\ checking/volumetriclight.xml
  effects/shaders/noise.frag
  effects/shaders/noise.xml
  effects/shaders/pixelate.frag
  effects/shaders/pixelate.xml
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
  effects/shaders/vignette.frag
  effects/shaders/vignette.xml
  effects/shaders/wave.frag
  effects/shaders/wave.xml
)

qt5_create_translation(OLIVE_QM_FILES
  ${CMAKE_SOURCE_DIR}
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
)

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

if(DOXYGEN_FOUND AND BUILD_DOXYGEN)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()
