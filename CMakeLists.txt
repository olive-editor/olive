# Olive - Non-Linear Video Editor
# Copyright (C) 2023 Olive Studios LLC
# Modifications Copyright (C) 2025 mikesolar
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

project(olive-editor VERSION 0.4.1 LANGUAGES CXX)
# Fallback version used only when git is unavailable (e.g. source tarball);
# normally overridden by the git tag / commit hash logic further below.
# Edit version.txt to change it.
if(EXISTS "${CMAKE_SOURCE_DIR}/version.txt")
  file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" PROJECT_VERSION LIMIT_COUNT 1)
  string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
else()
  set(PROJECT_VERSION "0.0.0-unknown")
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOFX_SUPPORTS_OPENGLRENDER")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOFX_SUPPORTS_OPENGLRENDER")
option(BUILD_QT6 "Build with Qt 6 over 5 (experimental)" ON)
option(BUILD_DOXYGEN "Build Doxygen documentation" OFF)
option(BUILD_TESTS "Build unit tests" OFF)
option(USE_WERROR "Error on compile warning" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Generates a compile_commands.json in the build dir, link that to the repo
# root to enrich your IDE with clangd language server protocol functionalities
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sanitizers
add_library(olive-sanitizers INTERFACE)
include(cmake/Sanitizers.cmake)
enable_sanitizers(olive-sanitizers)
list(APPEND OLIVE_LIBRARIES olive-sanitizers)

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
    /DOFX_SUPPORTS_OPENGLRENDER
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
    -DOFX_SUPPORTS_OPENGLRENDER

  )
  if (USE_WERROR)
    list(APPEND OLIVE_COMPILE_OPTIONS "-Werror")
  endif()
endif()

# MinGW does not define WIN32/WINDOWS the way OpenFX HostSupport expects.
# Ensure these are visible so ofxhBinary.h et al. pick the Windows code path.
if(MINGW)
  add_compile_definitions(WIN32 WINDOWS)
endif()

set(OLIVE_DEFINITIONS -DQT_DEPRECATED_WARNINGS)
if (WIN32)
  list(APPEND OLIVE_DEFINITIONS -DUNICODE -D_UNICODE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# OFX HostSupport requires expat::expat
find_package(EXPAT REQUIRED)
if (TARGET EXPAT::EXPAT AND NOT TARGET expat::expat)
  add_library(expat::expat ALIAS EXPAT::EXPAT)
endif()

# Link OpenGL
if(UNIX AND NOT APPLE AND NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()
find_package(OpenGL REQUIRED)
list(APPEND OLIVE_LIBRARIES OpenGL::GL)

# Optional: Link Vulkan and shaderc (for the Vulkan render backend)
find_package(Vulkan)
find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(SHADERC shaderc)
endif()
if(Vulkan_FOUND)
  message(STATUS "Vulkan found: ${Vulkan_LIBRARIES}")
else()
  message(STATUS "   Vulkan not found. The Vulkan render backend will be disabled.")
endif()

# Link OpenColorIO
find_package(OpenColorIO 2.1.1 REQUIRED)
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

# Link Olive
list(APPEND OLIVE_LIBRARIES olivecore)
list(APPEND OLIVE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/core/include)


# Link Qt
set(QT_LIBRARIES
  Core
  Gui
  Widgets
  OpenGL
  LinguistTools
  Concurrent
)
if (UNIX AND NOT APPLE)
  list(APPEND QT_LIBRARIES DBus)
endif()
if (BUILD_QT6)
  set(QT_NAME Qt6)
else()
  set(QT_NAME Qt5)
endif()
find_package(QT
  NAMES
    ${QT_NAME}
  REQUIRED
  COMPONENTS
    ${QT_LIBRARIES}
  OPTIONAL_COMPONENTS
    Network
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED
  COMPONENTS
    ${QT_LIBRARIES}
  OPTIONAL_COMPONENTS
    Network
)
if (NOT Qt${QT_VERSION_MAJOR}Network_FOUND)
  message("   Qt${QT_VERSION_MAJOR}::Network module not found, crash reporting will be disabled.")
endif()
list(APPEND OLIVE_LIBRARIES
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::OpenGL
  Qt${QT_VERSION_MAJOR}::Concurrent
)

if (${QT_VERSION_MAJOR} EQUAL "6")
  find_package(Qt${QT_VERSION_MAJOR}
    REQUIRED
      OpenGLWidgets
  )

  list(APPEND OLIVE_LIBRARIES
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
  )

  # Link KDDockWidgets
  #find_package(KDDockWidgets-qt6 CONFIG REQUIRED)
else()
  # Link KDDockWidgets
  #find_package(KDDockWidgets CONFIG REQUIRED)
endif()

list(APPEND OLIVE_LIBRARIES
  KDAB::kddockwidgets
)

# Link OFX HostSupport wherever libolive-editor objects are used.
list(APPEND OLIVE_LIBRARIES OfxHost)

# FFmpeg isolation: all FFmpeg access in the editor goes through this shared
# library's pure C API. It is the only component that links FFmpeg.
add_subdirectory(ffmpeg_bridge)
list(APPEND OLIVE_LIBRARIES ffmpeg_bridge)
list(APPEND OLIVE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_bridge/include)

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

if (APPLE)
  list(APPEND OLIVE_LIBRARIES "-framework IOKit")
elseif(UNIX)
  list(APPEND OLIVE_LIBRARIES Qt${QT_VERSION_MAJOR}::DBus)
endif()

# Determine version from git: the tag name if HEAD is exactly on a tag,
# otherwise the first 8 hex digits of the commit hash. Falls back to
# version.txt (read above) when git is unavailable (e.g. tarball).
set(PROJECT_LONG_VERSION ${PROJECT_VERSION})
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_TAG
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )

    if(GIT_TAG)
      # Tags are named like "v0.4.1-alpha"; drop the leading "v"
      string(REGEX REPLACE "^v" "" PROJECT_VERSION "${GIT_TAG}")
    else()
      execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE PROJECT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endif()

    set(PROJECT_LONG_VERSION ${PROJECT_VERSION})
  endif()
endif()

# Optional: Find Doxygen if requested
if(BUILD_DOXYGEN)
  find_package(Doxygen)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
list(APPEND OLIVE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party)

add_subdirectory(core EXCLUDE_FROM_ALL)

set(KDDockWidgets_STATIC ON CACHE INTERNAL "Force KDDockWidgets to build statically")
set(KDDockWidgets_QT6 ${BUILD_QT6} CACHE INTERNAL "Conform KDDockWidgets' Qt 6 setting to ours")
# Oak only uses the QtWidgets frontend; building the QtQuick frontend causes
# duplicate QML module registration on macOS and pulls in unused dependencies.
set(KDDockWidgets_FRONTENDS "qtwidgets" CACHE INTERNAL "Only build the QtWidgets frontend for Oak")
add_subdirectory(third_party/KDDockWidgets EXCLUDE_FROM_ALL)

add_subdirectory(third_party/openfx/HostSupport)
add_subdirectory(app)

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# ------------------------------------------------------------------------------
# CPack / system package configuration
# ------------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "oak-video-editor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Oak Video Editor Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Oak - Non-linear video editor")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://oakvideoeditor.org")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

# Debian
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Oak Video Editor Team")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
  "libqt6core6, libqt6gui6, libqt6widgets6, libqt6opengl6, libqt6openglwidgets6, libqt6network6, libqt6concurrent6, libavcodec60, libavformat60, libavutil58, libswscale7, libswresample4, libavfilter9, libopenimageio2.4, libopencolorio2, libopenexr-3-1-30, libexpat1, libportaudio2, libgl1, libvulkan1, libxkbcommon0")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_URL "https://oakvideoeditor.org")
set(CPACK_RPM_PACKAGE_REQUIRES
  "qt6-qtbase >= 6.0, qt6-qtbase-gui >= 6.0, qt6-qttools, ffmpeg-libs >= 6.0, OpenImageIO >= 2.4, OpenColorIO >= 2.0, openexr >= 3.1, expat, portaudio, mesa-libGL, vulkan-loader, libxkbcommon")

include(CPack)

