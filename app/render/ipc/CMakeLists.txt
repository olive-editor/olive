# Oak - Non-Linear Video Editor
# Copyright (C) 2026 Oak Team
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
        render/ipc/frameslotpool.cpp
        render/ipc/frameslotpool.h
        render/ipc/ipcmessage.cpp
        render/ipc/ipcmessage.h
        render/ipc/sharedmemoryregion.cpp
        render/ipc/sharedmemoryregion.h
        render/ipc/spscringbuffer.h
        PARENT_SCOPE
)