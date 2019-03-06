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

#include "crossplatformlib.h"

#include <QDebug>

ModulePtr LibLoad(const QString &filename) {
#ifdef _WIN32
	LPCWSTR dll_fn_w = reinterpret_cast<const wchar_t*>(filename.utf16());
	return LoadLibrary(dll_fn_w);
#elif defined(LINUX) || defined(APPLE) || defined(HAIKU)
	return dlopen(filename.toUtf8(), RTLD_LAZY);
#else
	qWarning() << "Olive doesn't know how to open dynamic libraries on this platform, external libraries will not be functional";
	return nullptr;
#endif
}

QStringList LibFilter() {
#ifdef _WIN32
	return QStringList("*.dll");
#elif defined(LINUX) || defined(APPLE) || defined(HAIKU)
    return {"*.so", "*.dylib"};
#endif
}

#ifdef APPLE
CFBundleRef BundleLoad(const QString &filename) {
    CFStringRef bundle_str = CFStringCreateWithCString(NULL, filename.toUtf8(), kCFStringEncodingUTF8);
    CFURLRef bundle_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, bundle_str, kCFURLPOSIXPathStyle, true);
    CFBundleRef bundle = NULL;
    if (bundle_url != NULL) {
        bundle = CFBundleCreate(kCFAllocatorDefault, bundle_url);
    } else {
        qCritical() << "Failed to create VST URL";
    }
    CFRelease(bundle_url);
    CFRelease(bundle_str);
    return bundle;
}

void BundleClose(CFBundleRef bundle) {
    CFBundleUnloadExecutable(bundle);
    CFRelease(bundle);
}
#endif
