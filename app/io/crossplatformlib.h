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

#ifndef CROSSPLATFORMLIB_H
#define CROSSPLATFORMLIB_H

#include <QString>

#ifdef _WIN32
	#include <Windows.h>
	#define LibAddress GetProcAddress
	#define LibClose FreeModule
	#define ModulePtr HMODULE
#elif defined(__linux__) || defined(__APPLE__)
	#include <dlfcn.h>
	#define LibAddress dlsym
	#define LibClose dlclose
	#define ModulePtr void*
#endif

ModulePtr LibLoad(const QString& filename);
QStringList LibFilter();

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
class NSWindow;

CFBundleRef BundleLoad(const QString& filename);
void BundleClose(CFBundleRef bundle);
#endif

#endif // CROSSPLATFORMLIB_H
