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
  node/processor/audiorenderer/audiorenderer.h
  node/processor/audiorenderer/audiorenderer.cpp
  node/processor/audiorenderer/audiorendererparams.h
  node/processor/audiorenderer/audiorendererparams.cpp
  node/processor/audiorenderer/audiorendererthread.h
  node/processor/audiorenderer/audiorendererthread.cpp
  node/processor/audiorenderer/audiorendererthreadbase.h
  node/processor/audiorenderer/audiorendererthreadbase.cpp
  PARENT_SCOPE
)
