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

add_subdirectory(job)
add_subdirectory(ocioconf)
add_subdirectory(opengl)

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  render/audioparams.cpp
  render/audioparams.h
  render/audioplaybackcache.cpp
  render/audioplaybackcache.h
  render/color.cpp
  render/color.h
  render/colormanager.cpp
  render/colormanager.h
  render/colorprocessor.cpp
  render/colorprocessor.h
  render/colorprocessorcache.h
  render/diskmanager.cpp
  render/diskmanager.h
  render/framehashcache.cpp
  render/framehashcache.h
  render/managedcolor.cpp
  render/managedcolor.h
  render/pixelformat.cpp
  render/pixelformat.h
  render/playbackcache.cpp
  render/playbackcache.h
  render/previewautocacher.cpp
  render/previewautocacher.h
  render/renderer.cpp
  render/renderer.h
  render/rendercache.h
  render/rendererthreadwrapper.cpp
  render/rendererthreadwrapper.h
  render/rendermanager.cpp
  render/rendermanager.h
  render/rendermodes.h
  render/renderprocessor.cpp
  render/renderprocessor.h
  render/shadercode.h
  render/shadervalue.h
  render/stillimagecache.h
  render/texture.cpp
  render/texture.h
  render/videoparams.cpp
  render/videoparams.h
  PARENT_SCOPE
)

set(OLIVE_RESOURCES
  ${OLIVE_RESOURCES}
  PARENT_SCOPE
)
