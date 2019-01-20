#include "crossplatformlib.h"

#include <QDebug>

void *LibLoad(const QString &filename) {
#ifdef _WIN32
    LPCWSTR dll_fn_w = reinterpret_cast<const wchar_t*>(filename.utf16());
    return LoadLibrary(dll_fn_w);
#elif __linux__
    return dlopen(filename.toUtf8(), RTLD_LAZY);
#else
    qWarning() << "Olive doesn't know how to open dynamic libraries on this platform, external libraries will not be functional";
    return nullptr;
#endif
}

QStringList LibFilter() {
#ifdef _WIN32
    return QStringList("*.dll");
#elif __linux__
    return QStringList("*.so");
#elif __APPLE__
    return QStringList("*.dylib");
#endif
}
