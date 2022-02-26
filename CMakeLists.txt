# Olive - Non-Linear Video Editor
# Copyright (C) 2021 Olive Team
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

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(olive-editor VERSION 0.2.0 LANGUAGES CXX)

option(BUILD_DOXYGEN "Build Doxygen documentation" OFF)
option(BUILD_TESTS "Build unit tests" ON)
option(USE_WERROR "Error on compile warning" ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set compiler options
if(MSVC)
  set(OLIVE_COMPILE_OPTIONS
    /wd4267
    /wd4244
    /experimental:external
    /external:anglebrackets
    /external:W0
    "$<$<CONFIG:RELEASE>:/O2>"
    "$<$<COMPILE_LANGUAGE:CXX>:/MP>"
  )
  if (USE_WERROR)
    list(APPEND OLIVE_COMPILE_OPTIONS "/WX")
  endif()
else()
  set(OLIVE_COMPILE_OPTIONS
    "$<$<CONFIG:RELEASE>:-O2>"
    -Wuninitialized
    -pedantic-errors
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wshadow
  )
  if (USE_WERROR)
    list(APPEND OLIVE_COMPILE_OPTIONS "-Werror")
  endif()
endif()

set(OLIVE_DEFINITIONS -DQT_DEPRECATED_WARNINGS)
if (WIN32)
  list(APPEND OLIVE_DEFINITIONS -DUNICODE -D_UNICODE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Link OpenGL
if(UNIX AND NOT APPLE AND NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()
find_package(OpenGL REQUIRED)
list(APPEND OLIVE_LIBRARIES OpenGL::GL)

# Link OpenColorIO
find_package(OpenColorIO 2.0.0 REQUIRED)
list(APPEND OLIVE_LIBRARIES ${OCIO_LIBRARIES})
list(APPEND OLIVE_INCLUDE_DIRS ${OCIO_INCLUDE_DIRS})

# Link OpenImageIO
find_package(OpenImageIO 2.1.12 REQUIRED)
list(APPEND OLIVE_LIBRARIES ${OIIO_LIBRARIES})
list(APPEND OLIVE_INCLUDE_DIRS ${OIIO_INCLUDE_DIRS})

# Link OpenEXR
find_package(OpenEXR REQUIRED)
list(APPEND OLIVE_LIBRARIES ${OPENEXR_LIBRARIES})
list(APPEND OLIVE_INCLUDE_DIRS ${OPENEXR_INCLUDES})

# Link Qt 5
set(QT_LIBRARIES
  Core
  Gui
  Widgets
  OpenGL
  Svg
  LinguistTools
  Concurrent
)
if (UNIX AND NOT APPLE)
  list(APPEND QT_LIBRARIES DBus)
endif()
find_package(Qt5 5.6 REQUIRED
  COMPONENTS
    ${QT_LIBRARIES}
  OPTIONAL_COMPONENTS
    Network
)
if (NOT Qt5Network_FOUND)
  message("   Qt5::Network module not found, crash reporting will be disabled.")
endif()
list(APPEND OLIVE_LIBRARIES
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::OpenGL
  Qt5::Svg
  Qt5::Concurrent
)

# Link FFmpeg
find_package(FFMPEG 3.0 REQUIRED
  COMPONENTS
  avutil
  avcodec
  avformat
  avfilter
  swscale
  swresample
)
list(APPEND OLIVE_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
list(APPEND OLIVE_LIBRARIES
  FFMPEG::avutil
  FFMPEG::avcodec
  FFMPEG::avformat
  FFMPEG::avfilter
  FFMPEG::swscale
  FFMPEG::swresample
)

# Link PortAudio
find_package(PortAudio REQUIRED)
set(CMAKE_REQUIRED_INCLUDES ${PORTAUDIO_INCLUDE_DIRS})
include(CheckIncludeFileCXX)
check_include_file_cxx( "pa_jack.h" PA_HAS_JACK)
if (PA_HAS_JACK)
  list(APPEND OLIVE_DEFINITIONS PA_HAS_JACK)
endif()

list(APPEND OLIVE_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIRS})
list(APPEND OLIVE_LIBRARIES ${PORTAUDIO_LIBRARIES})

# Optional: Link OpenTimelineIO
find_package(OpenTimelineIO)
if (OpenTimelineIO_FOUND)
  list(APPEND OLIVE_DEFINITIONS USE_OTIO)
  list(APPEND OLIVE_INCLUDE_DIRS ${OTIO_INCLUDE_DIRS})
  list(APPEND OLIVE_LIBRARIES ${OTIO_LIBRARIES})
else()
  message("   OpenTimelineIO interchange will be disabled.")
endif()

# Optional: Link Google Crashpad
find_package(GoogleCrashpad)
if (GoogleCrashpad_FOUND)
  list(APPEND OLIVE_DEFINITIONS USE_CRASHPAD)
  list(APPEND OLIVE_INCLUDE_DIRS ${CRASHPAD_INCLUDE_DIRS})
  list(APPEND OLIVE_LIBRARIES ${CRASHPAD_LIBRARIES})
else()
  message("   Automatic crash reporting will be disabled.")

  if (APPLE)
    # Enables use of special functions for slider dragging, only linked if Crashpad isn't found
    # because Crashpad links it itself and will cause duplicate references if we also link it
    list(APPEND OLIVE_LIBRARIES "-framework ApplicationServices")
  endif()
endif()

if (WIN32)
  list(APPEND OLIVE_DEFINITIONS "-DUNICODE -D_UNICODE")
elseif (APPLE)
  list(APPEND OLIVE_LIBRARIES "-framework IOKit")
elseif(UNIX)
  list(APPEND OLIVE_LIBRARIES Qt5::DBus)
endif()

# Generate Git hash
set(PROJECT_LONG_VERSION ${PROJECT_VERSION})
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(PROJECT_LONG_VERSION ${PROJECT_VERSION}-${GIT_HASH})
  endif()
endif()

# Optional: Find Doxygen if requested
if(BUILD_DOXYGEN)
  find_package(Doxygen)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
list(APPEND OLIVE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/ext)

add_subdirectory(app)

if (BUILD_TESTS)
enable_testing()
add_subdirectory(tests)
endif()
