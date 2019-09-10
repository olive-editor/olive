#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

    # Generate Makefile
    cmake .

    # Make
    make -j$(sysctl -n hw.ncpu)

    # Move Qt deps into bundle
    macdeployqt Olive.app

    # Fix other deps that macdeployqt missed
    wget -c -nv https://github.com/arl/macdeployqtfix/raw/master/macdeployqtfix.py
    python2 macdeployqtfix.py Olive.app/Contents/MacOS/Olive /usr/local/Cellar/qt5/5.*/

    # Distribute in zip
    zip -r Olive-$(git rev-parse --short HEAD)-macOS.zip Olive.app

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    # Generate Makefile
    cmake .

    # Make
    make -j$(nproc)

    # Use `make install` on `appdir` to place files in the correct place
    make INSTALL_ROOT=appdir -j$(nproc) install ; find appdir/

    # Download linuxdeployqt
    wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod a+x linuxdeployqt-continuous-x86_64.AppImage

    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
    
    # linuxdeployqt uses this for naming the file
    export VERSION=$(git rev-parse --short HEAD)

    # Use linuxdeployqt to set up dependencies
    ./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -extra-plugins=imageformats/libqsvg.so -appimage

fi
