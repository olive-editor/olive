/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef PLATFORM_H
#define PLATFORM_H

#include <QtGlobal>

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64) || defined(Q_OS_WINCE) || defined(Q_OS_WINRT)
#define WINDOWS
#elif defined(Q_OS_LINUX) || defined(__linux__)
#define LINUX
#elif defined(__HAIKU__)
#define HAIKU
#elif defined(__APPLE__) || defined(Q_OS_DARWIN) || defined(Q_OS_MAC)
#define APPLE
#endif

#endif // PLATFORM_H