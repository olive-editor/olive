#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

    # Generate Makefile
    cmake .

    # Make
    make -j$(sysctl -n hw.ncpu)

    # Handle compile failure
    if [ "$?" != "0" ]
    then
        exit 1
    fi

    BUNDLE_PATH=$(find . -name "Olive.app")

    echo Found app at: $BUNDLE_PATH

    # Move Qt deps into bundle
    macdeployqt $BUNDLE_PATH

    # Fix other deps that macdeployqt missed
    wget -c -nv https://github.com/arl/macdeployqtfix/raw/master/macdeployqtfix.py
    python2 macdeployqtfix.py $BUNDLE_PATH/Contents/MacOS/Olive /usr/local/Cellar/qt5/5.*/

    # Distribute in zip
    zip -r Olive-$(git rev-parse --short HEAD)-macOS.zip $BUNDLE_PATH

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    # Generate Makefile
    cmake . -DQt5LinguistTools_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5LinguistTools

    # Make
    make -j$(nproc)

    # Handle compile failure
    if [ "$?" != "0" ]
    then
        exit 1
    fi

    # Use `make install` on `appdir` to place files in the correct place
    make DESTDIR=appdir -j$(nproc) install

    # Download linuxdeployqt
    wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod a+x linuxdeployqt-continuous-x86_64.AppImage

    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
    
    # linuxdeployqt uses this for naming the file
    export VERSION=$(git rev-parse --short HEAD)

    # Use linuxdeployqt to set up dependencies
    ./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/local/share/applications/*.desktop -extra-plugins=imageformats/libqsvg.so -appimage

fi
