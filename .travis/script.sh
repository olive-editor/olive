#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
	lrelease olive.pro
	qmake CONFIG+=release PREFIX=/usr
	make -j$(nproc)
	macdeployqt Olive.app
	zip -r Olive-$(git rev-parse --short HEAD)-macOS.zip Olive.app
elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
	lrelease olive.pro
	if [ "$ARCH" == "i386" ]; then
		qmake CONFIG+=release "QMAKE_CFLAGS+=-m32" "QMAKE_CXXFLAGS+=-m32" "QMAKE_LFLAGS+=-m32" PREFIX=/usr -spec linux-g++-32
	else
		qmake CONFIG+=release PREFIX=/usr
	fi
	make -j$(nproc)
	make INSTALL_ROOT=appdir -j$(nproc) install ; find appdir/
	wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
	chmod a+x linuxdeployqt-continuous-x86_64.AppImage
	unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
	export VERSION=$(git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
	./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage
	if [ "$ARCH" == "i386" ]; then
		rm Olive*.AppImage
		wget -c -nv "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-i686.AppImage"
		chmod a+x appimagetool-i686.AppImage
		./appimagetool-i686.AppImage "appdir" -n -g
	fi
fi
