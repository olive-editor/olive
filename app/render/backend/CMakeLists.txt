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

add_subdirectory(audio)
add_subdirectory(opengl)
add_subdirectory(vulkan)

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}

  render/backend/exporter.h
  render/backend/exporter.cpp

  render/backend/renderbackend.h
  render/backend/renderbackend.cpp
  render/backend/renderworker.h
  render/backend/renderworker.cpp

  render/backend/audiorenderbackend.h
  render/backend/audiorenderbackend.cpp
  render/backend/audiorenderworker.h
  render/backend/audiorenderworker.cpp
  
  render/backend/videorenderbackend.h
  render/backend/videorenderbackend.cpp
  render/backend/videorenderframecache.h
  render/backend/videorenderframecache.cpp
  render/backend/videorenderworker.h
  render/backend/videorenderworker.cpp

  render/backend/rendercache.h
  render/backend/colorprocessorcache.h
  render/backend/decodercache.h
  
  PARENT_SCOPE
)
