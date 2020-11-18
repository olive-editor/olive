# Olive - Non-Linear Video Editor
# Copyright (C) 2020 Olive Team
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

option(UPDATE_TS "Update translations" OFF)
option(BUILD_DOXYGEN "Build Doxygen documentation" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13")

set(OLIVE_DEFINITIONS -DAPPVERSION="${PROJECT_VERSION}" -DQT_DEPRECATED_WARNINGS)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(UNIX AND NOT APPLE AND NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()

find_package(OpenGL REQUIRED)

find_package(OpenColorIO 2.0.0 REQUIRED)

find_package(OpenImageIO 1.6 REQUIRED)

find_package(OpenEXR REQUIRED)

find_package(Qt5 5.6 REQUIRED
  COMPONENTS
  Core
  Gui
  Widgets
  Multimedia
  OpenGL
  Svg
  LinguistTools
  Concurrent
)

find_package(FFMPEG 3.0 REQUIRED
  COMPONENTS
  avutil
  avcodec
  avformat
  avfilter
  swscale
  swresample
)

find_package(OpenTimelineIO)

if (NOT OpenTimelineIO_FOUND)
  message("   OpenTimelineIO interchange will be disabled.")
endif()

find_package(GoogleCrashpad)

if (NOT GoogleCrashpad_FOUND)
  message("   Automatic crash reporting will be disabled.")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()
if(DEFINED GIT_HASH)
  message("Olive: Git hash = " "${GIT_HASH}")
  list(APPEND OLIVE_DEFINITIONS -DGITHASH="${GIT_HASH}")
else()
  message("Olive: No Git hash defined!")
endif()

if(BUILD_DOXYGEN)
  find_package(Doxygen)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(app)
