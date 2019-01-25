#include "crossplatformlib.h"

#include <QDebug>

ModulePtr LibLoad(const QString &filename) {
#ifdef _WIN32
	LPCWSTR dll_fn_w = reinterpret_cast<const wchar_t*>(filename.utf16());
	return LoadLibrary(dll_fn_w);
#elif defined(__linux__) || defined(__APPLE__)
	return dlopen(filename.toUtf8(), RTLD_LAZY);
#else
	qWarning() << "Olive doesn't know how to open dynamic libraries on this platform, external libraries will not be functional";
	return nullptr;
#endif
}

QStringList LibFilter() {
#ifdef _WIN32
	return QStringList("*.dll");
#elif defined(__linux__) || defined(__APPLE__)
    return {"*.so", "*.dylib"};
#endif
}

#ifdef __APPLE__
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
