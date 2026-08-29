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

target_sources(libolive-editor PRIVATE
        cancelableobject.h
        commandlineparser.cpp
        commandlineparser.h
        crashpadinterface.cpp
        crashpadinterface.h
        crashpadutils.h
        Current.cpp
        Current.h
        debug.cpp
        debug.h
        decibel.h
        define.h
        ffmpegutils.cpp
        ffmpegutils.h
        filefunctions.cpp
        filefunctions.h
        html.cpp
        html.h
        jobtime.cpp
        jobtime.h
        lerp.h
        memorypool.h
        ocioutils.cpp
        ocioutils.h
        oiioutils.cpp
        oiioutils.h
        otioutils.h
        qtutils.cpp
        qtutils.h
        range.h
        ratiodialog.cpp
        ratiodialog.h
        threadsafemap.h
        tohex.h
        util.h
        xmlutils.cpp
        xmlutils.h
)
