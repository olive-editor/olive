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

cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(olive-editor VERSION 0.2.0 LANGUAGES CXX)

option(UPDATE_TS "Update translations" OFF)
option(BUILD_DOXYGEN "Build Doxygen documentation" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(OLIVE_DEFINITIONS -DAPPVERSION="${PROJECT_VERSION}" -DQT_DEPRECATED_WARNINGS)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(UNIX AND NOT APPLE AND NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()

find_package(OpenGL REQUIRED)

find_package(OpenColorIO REQUIRED)

find_package(OpenImageIO REQUIRED)

find_package(Qt5 5.6 REQUIRED
  COMPONENTS
  Core
  Gui
  Widgets
  Multimedia
  OpenGL
  Svg
  LinguistTools
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

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
elseif(UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
  # Fallback for Ubuntu/Launchpad (extracts Git hash from debian/changelog rather than Git repo)
  # (see https://answers.launchpad.net/launchpad/+question/678556)
  execute_process(COMMAND sh -c "grep -Po '(?<=-)(([a-z0-9])\\w+)(?=\\+)' -m 1 changelog"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/debian
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
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
