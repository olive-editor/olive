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
  node/processor/videorenderer/videorenderer.h
  node/processor/videorenderer/videorenderer.cpp
  node/processor/videorenderer/videorendererthreadbase.h
  node/processor/videorenderer/videorendererthreadbase.cpp
  node/processor/videorenderer/videorendererdownloadthread.h
  node/processor/videorenderer/videorendererdownloadthread.cpp
  node/processor/videorenderer/videorendererprocessthread.h
  node/processor/videorenderer/videorendererprocessthread.cpp
  PARENT_SCOPE
)
