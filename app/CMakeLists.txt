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
)

add_subdirectory(audio)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(decoder)
add_subdirectory(dialog)
add_subdirectory(node)
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

if(APPLE)
  SET_TARGET_PROPERTIES(${OLIVE_TARGET} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_FRAMEWORK_IDENTIFIER org.olivevideoeditor.Olive
  )
endif()

target_compile_definitions(${OLIVE_TARGET} PRIVATE ${OLIVE_DEFINITIONS})

if(MSVC)
  # Enabling this threw warnings from FFmpeg headers that broke compilation. See if this is fixable.
  #target_compile_options(${OLIVE_TARGET} PRIVATE /W4 /WX)
else()
  target_compile_options(${OLIVE_TARGET} PRIVATE -Werror -Wuninitialized -pedantic-errors -Wall -Wextra -Wconversion -Wsign-conversion)
endif()

target_include_directories(
  ${OLIVE_TARGET}
  PRIVATE
  ${OPENCOLORIO_INCLUDE_DIR}
  ${OIIO_INCLUDE_DIRS}
  ${FFMPEG_INCLUDE_DIRS}
)

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
  ${OPENCOLORIO_LIBRARIES}
  ${OIIO_LIBRARIES}
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
  install(FILES ${OLIVE_QM_FILES} DESTINATION share/olive-editor/ts)
endif()

add_subdirectory(packaging)

if(DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()
