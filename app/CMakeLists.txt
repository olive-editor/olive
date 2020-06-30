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

# Set Olive sources and resources
set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  core.h
  core.cpp
  main.cpp
)

if (WIN32)
  set(OLIVE_RESOURCES
    ${OLIVE_RESOURCES}
    packaging/windows/resources.rc
  )
endif()

add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(dialog)
add_subdirectory(node)
add_subdirectory(panel)
add_subdirectory(project)
add_subdirectory(render)
add_subdirectory(shaders)
add_subdirectory(task)
add_subdirectory(timeline)
add_subdirectory(tool)
add_subdirectory(ui)
add_subdirectory(undo)
add_subdirectory(widget)
add_subdirectory(window)

# Create main application target
set(OLIVE_TARGET "olive-editor")
if(APPLE)
  set(OLIVE_TARGET "Olive")

  set(OLIVE_ICON packaging/macos/olive.icns)

  set(OLIVE_RESOURCES
    ${OLIVE_RESOURCES}
    ${OLIVE_ICON}
  )
endif()

# Add executable
add_executable(${OLIVE_TARGET}
  ${OLIVE_SOURCES}
  ${OLIVE_RESOURCES}
  ${OLIVE_QM_FILES}
)

if(APPLE)
  set_target_properties(${OLIVE_TARGET} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER org.olivevideoeditor.Olive
    MACOSX_BUNDLE_ICON_FILE olive.icns
    RESOURCE "${OLIVE_ICON}"
  )

  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
endif()

# Set compiler definitions
target_compile_definitions(${OLIVE_TARGET} PRIVATE ${OLIVE_DEFINITIONS})

# Set compiler options
if(MSVC)
  target_compile_options(
    ${OLIVE_TARGET}
    PRIVATE
    /WX
    /wd4267
    /wd4244
    /experimental:external
    /external:anglebrackets
    /external:W0
    "$<$<CONFIG:RELEASE>:/O2>"
    "$<$<COMPILE_LANGUAGE:CXX>:/MP>"
  )
else()
  target_compile_options(
    ${OLIVE_TARGET}
    PRIVATE
    -O2
    -Werror
    -Wuninitialized
    -pedantic-errors
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wshadow
  )
endif()

if(UNIX AND NOT APPLE)
  target_link_options(
    ${OLIVE_TARGET}
    PRIVATE
    -rdynamic
  )
endif()

# Set include directories
target_include_directories(
  ${OLIVE_TARGET}
  PRIVATE
  ${FFMPEG_INCLUDE_DIRS}
  ${OCIO_INCLUDE_DIRS}
  ${OIIO_INCLUDE_DIRS}
  ${OPENEXR_INCLUDE_DIRS}
)

# Set link libraries
target_link_libraries(
  ${OLIVE_TARGET}
  PRIVATE
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Multimedia
  Qt5::OpenGL
  Qt5::Svg
  Qt5::Concurrent
  OpenGL::GL
  FFMPEG::avutil
  FFMPEG::avcodec
  FFMPEG::avformat
  FFMPEG::avfilter
  FFMPEG::swscale
  FFMPEG::swresample
  ${OCIO_LIBRARIES}
  ${OIIO_LIBRARIES}
  ${OPENEXR_LIBRARIES}
)

if (WIN32)
  target_link_libraries(
    ${OLIVE_TARGET}
    PRIVATE
    DbgHelp
  )
elseif (APPLE)
  target_link_libraries(
    ${OLIVE_TARGET}
    PRIVATE
    "-framework ApplicationServices"
  )
endif()

set(OLIVE_TS_FILES
  # FIXME: Empty variable
)

if(UPDATE_TS)
  qt5_create_translation(OLIVE_QM_FILES ${CMAKE_SOURCE_DIR} ${OLIVE_TS_FILES})
else()
  qt5_add_translation(OLIVE_QM_FILES ${OLIVE_TS_FILES})
endif()

add_subdirectory(packaging)

if(DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()

set(OLIVE_CRASH_TARGET "olive-crashhandler")

set(OLIVE_CRASH_SOURCES
  dialog/crashhandler/crashhandler.h
  dialog/crashhandler/crashhandler.cpp
  dialog/crashhandler/crashhandlermain.cpp
)

if (WIN32)
  add_executable(
    ${OLIVE_CRASH_TARGET}
    WIN32
    ${OLIVE_CRASH_SOURCES}
  )
else()
  add_executable(
    ${OLIVE_CRASH_TARGET}
    ${OLIVE_CRASH_SOURCES}
  )
endif()

target_link_libraries(
  ${OLIVE_CRASH_TARGET}
  PRIVATE
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
)

if(UNIX AND NOT APPLE)
  install(TARGETS ${OLIVE_TARGET} ${OLIVE_CRASH_TARGET} RUNTIME DESTINATION bin)
endif()

if(APPLE)
  # Move crash handler program inside Mac app bundle
  add_custom_command(TARGET ${OLIVE_CRASH_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${OLIVE_CRASH_TARGET} $<TARGET_FILE_DIR:${OLIVE_TARGET}>
  )
endif()
