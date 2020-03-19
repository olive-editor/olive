#!/bin/bash

# linuxdeployqt uses this for naming the file
export VERSION=$(git rev-parse --short=8 HEAD)

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

    BUNDLE_NAME=Olive.app

    # Move bundle to working directory
    mv app/$BUNDLE_NAME .

    # Move Qt deps into bundle
    macdeployqt $BUNDLE_NAME

    # Fix other deps that macdeployqt missed
    wget -c -nv https://github.com/arl/macdeployqtfix/raw/master/macdeployqtfix.py
    python2 macdeployqtfix.py $BUNDLE_NAME/Contents/MacOS/Olive /usr/local/Cellar/qt5/5.*/

    # Fix deps on crash handler
    python2 macdeployqtfix.py $BUNDLE_NAME/Contents/MacOS/olive-crashhandler /usr/local/Cellar/qt5/5.*/

    # Fix OpenEXR libs that seem to be missed by both macdeployqt _and_ macdeployqtfix
    cd $BUNDLE_NAME/Contents/Frameworks
    exrlib=(libImath-*.dylib libHalf-*.dylib libIexMath-*.dylib libIex-*.dylib libIlmThread-*.dylib)

    for a in ${exrlib[@]}; do
        for b in ${exrlib[@]}; do
            install_name_tool -change @rpath/$b @executable_path/../Frameworks/$b $a
        done
    done

    cd ../../..

    # Distribute in zip
    zip -r Olive-$VERSION-macOS.zip $BUNDLE_NAME

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    # Generate Makefile
    cmake .

    # Make
    make -j$(nproc)

    # Handle compile failure
    if [ "$?" != "0" ]
    then
        exit 1
    fi

    # Use `make install` on `appdir` to place files in the correct place
    make DESTDIR=appdir install

    # Download linuxdeployqt
    wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod a+x linuxdeployqt-continuous-x86_64.AppImage

    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH

    # Use linuxdeployqt to set up dependencies
    ./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/local/share/applications/*.desktop -extra-plugins=imageformats/libqsvg.so -appimage

fi
