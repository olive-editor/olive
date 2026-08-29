

#ifndef OFX_BINARY_H
#define OFX_BINARY_H

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause
#include <string>
#include <iostream>

#if defined(WIN32) || defined(WIN64)
#define I386
#elif defined(__linux__) || defined(__FreeBSD__)
#define UNIX
#ifdef __i386__
#define I386
#elif defined(__amd64__)
#define AMD64
#else
#error cannot detect architecture
#endif
#elif defined( __APPLE__)
#define UNIX
#else
#error cannot detect operating system
#endif

#if defined(UNIX)
#include <dlfcn.h>
#elif defined (WINDOWS)
#include "windows.h"
#include <assert.h>
#endif

#include <sys/stat.h>

namespace OFX 
{

  /// class representing a DLL/Shared Object/etc
  class Binary {
    /// destruction will close the library and invalidate
    /// any function pointers returned by lookupSymbol()
  protected :
    std::string _binaryPath;
    bool _invalid;
#if defined(UNIX)
    void *_dlHandle;
#elif defined (WINDOWS)
    HINSTANCE _dlHandle;
#endif
    time_t _time;
    off_t _size;
    int _users;
  public :

    /// create object representing the binary.  will stat() it, 
    /// and this fails, will set binary to be invalid.
    Binary(const std::string &binaryPath);

    ~Binary() { unload(); }

    bool isLoaded() const { return _dlHandle != 0; }

    /// is this binary invalid? (did the a stat() or load() on the file fail,
    /// or are we missing a some of the symbols?
    bool isInvalid() const { return _invalid; }

    /// set invalid status (e.g. called by user if a mandatory symbol was missing)
    void setInvalid(bool invalid) { _invalid = invalid; }

    /// Last modification time of the file.
    time_t getTime() const { return _time; }

    /// Current size of the file.
    off_t getSize() const { return _size; }

    /// Path to the file.
    const std::string &getBinaryPath() const { return _binaryPath; }

    void ref();
    void unref();

    /// open the binary.
    void load();

    /// close the binary
    void unload();

    /// look up a symbol in the binary file and return it as a pointer.
    /// returns null pointer if not found, or if the library is not loaded.
    void *findSymbol(const std::string &symbol);
  };
}

#endif
