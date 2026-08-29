# libolivecore
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

project(libolivecore VERSION 1.0.0 LANGUAGES CXX)

option(OLIVECORE_BUILD_TESTS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Link Imath
find_package(Imath REQUIRED CONFIG)

# Link OpenGL
if(UNIX AND NOT APPLE AND NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()
find_package(OpenGL REQUIRED)

add_library(olivecore
  src/render/audioparams.cpp
  src/render/samplebuffer.cpp
  src/util/bezier.cpp
  src/util/color.cpp
  src/util/fractionutils.cpp
  src/util/rational.cpp
  src/util/stringutils.cpp
  src/util/tests.cpp
  src/util/timecodefunctions.cpp
  src/util/timerange.cpp
  src/util/value.cpp
)

target_include_directories(olivecore PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include/olive/core"
  "${CMAKE_SOURCE_DIR}/third_party/openfx/include/"
)

target_link_libraries(olivecore PRIVATE
  OpenGL::GL
  Imath::Imath
)

# Link OpenTimelineIO (optional)
find_package(OpenTimelineIO)
if (OpenTimelineIO_FOUND)
  target_compile_definitions(olivecore PRIVATE USE_OTIO)
  target_include_directories(olivecore PRIVATE ${OTIO_INCLUDE_DIRS})
  target_link_libraries(olivecore PRIVATE ${OTIO_LIBRARIES})
else()
  message("   OpenTimelineIO interchange will be disabled.")
endif()

install(TARGETS olivecore)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/olive" DESTINATION "${CMAKE_INSTALL_PREFIX}/include")

if (OLIVECORE_BUILD_TESTS)
  enable_testing()

  function(make_test name)
    add_executable(${name}
      tests/${name}.cpp
    )
    target_link_libraries(${name} PRIVATE olivecore)
    target_include_directories(${name} PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/olive/core"
    )
    add_test(${name} ${name})
  endfunction()

  make_test(rational-test)
  make_test(stringutils-test)
  make_test(timecode-test)
  make_test(timerange-test)
endif()
