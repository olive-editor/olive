# Olive - Non-Linear Video Editor
# Copyright (C) 2022 Olive Team
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

# ffmpeg_bridge: a self-contained shared library that isolates all FFmpeg
# access behind a pure C API (see include/ffmpeg_bridge/ffmpeg_bridge.h).
# The editor and the render worker link only against this library.

find_package(FFMPEG 6.0 REQUIRED
  COMPONENTS
  avutil
  avcodec
  avformat
  avfilter
  swscale
  swresample
)

add_library(ffmpeg_bridge SHARED
  src/audiograph.cpp
  src/decoder.cpp
  src/encoder.cpp
  src/frame.cpp
  src/packet.cpp
  src/probe.cpp
  src/sws.cpp
  src/swr.cpp
  src/utils.cpp
  src/internal.h
  include/ffmpeg_bridge/ffmpeg_bridge.h
)

target_compile_features(ffmpeg_bridge PRIVATE cxx_std_17)

target_include_directories(ffmpeg_bridge
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(ffmpeg_bridge
  PRIVATE
    FFMPEG::avutil
    FFMPEG::avcodec
    FFMPEG::avformat
    FFMPEG::avfilter
    FFMPEG::swscale
    FFMPEG::swresample
)

# Static FFmpeg (e.g. the Linux CI build) needs system libs for
# libavcodec/libavformat.
find_package(ZLIB REQUIRED)
target_link_libraries(ffmpeg_bridge PRIVATE ZLIB::ZLIB)

find_package(BZip2 REQUIRED)
target_link_libraries(ffmpeg_bridge PRIVATE BZip2::BZip2)

find_package(LibLZMA)
if (LIBLZMA_FOUND)
  target_link_libraries(ffmpeg_bridge PRIVATE ${LIBLZMA_LIBRARIES})
endif()

target_compile_definitions(ffmpeg_bridge PRIVATE FFMPEG_BRIDGE_BUILD)

# Only export the FB_API functions
set_target_properties(ffmpeg_bridge PROPERTIES
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

# The shared library lives in its own directory outside of app/
set_target_properties(ffmpeg_bridge PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ffmpeg_bridge/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ffmpeg_bridge/bin"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ffmpeg_bridge/lib"
)

if (WIN32)
  set_target_properties(ffmpeg_bridge PROPERTIES PREFIX "")
  # Windows has no RPATH: the DLL must sit next to the executables
  install(TARGETS ffmpeg_bridge
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION ffmpeg_bridge/lib
  )
else()
  install(TARGETS ffmpeg_bridge
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION ffmpeg_bridge/lib
  )
endif ()
