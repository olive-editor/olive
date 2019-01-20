#ifndef CROSSPLATFORMLIB_H
#define CROSSPLATFORMLIB_H

#include <QString>

#ifdef _WIN32
    #include <Windows.h>
    #define LibAddress GetProcAddress
    #define CloseLib FreeModule
    #define ModulePtr HMODULE
#elif __linux__
    #include <dlfcn.h>
    #define LibAddress dlsym
    #define LibClose dlclose
    #define ModulePtr void*
#endif

ModulePtr LibLoad(const QString& filename);
QStringList LibFilter();

#endif // CROSSPLATFORMLIB_H
