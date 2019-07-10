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
  core.h
  core.cpp
  main.cpp
  rational.h
  rational.cpp
)

add_subdirectory(decoder)
add_subdirectory(panel)
add_subdirectory(project)
add_subdirectory(render)
add_subdirectory(task)
add_subdirectory(tool)
add_subdirectory(ui)
add_subdirectory(undo)
add_subdirectory(widget)
add_subdirectory(window)

set(OLIVE_TARGET "olive-editor")
if(APPLE)
  set(OLIVE_TARGET "Olive")
endif()

set(OLIVE_RESOURCES
  ${OLIVE_RESOURCES}
  #effects/internal/internalshaders.qrc
)

add_executable(${OLIVE_TARGET}
  ${OLIVE_SOURCES}
  ${OLIVE_RESOURCES}
  ${OLIVE_EFFECTS}
  ${OLIVE_QM_FILES}
)

target_compile_definitions(${OLIVE_TARGET} PRIVATE ${OLIVE_DEFINITIONS})

target_compile_options(
  ${OLIVE_TARGET}
  PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Werror -Wuninitialized -pedantic-errors -Wall -Wextra -Wconversion -Wsign-conversion>)

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

set(OLIVE_EFFECTS
  # FIXME: Empty variable
)

set(OLIVE_TS_FILES
  # FIXME: Empty variable
)

if(UPDATE_TS)
  qt5_create_translation(OLIVE_QM_FILES ${CMAKE_SOURCE_DIR} ${OLIVE_TS_FILES})
else()
  qt5_add_translation(OLIVE_QM_FILES ${OLIVE_TS_FILES})
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

if(DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()
