#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

	# generate translation files
	lrelease olive.pro

	# generate Makefile
	qmake CONFIG+=release PREFIX=/usr

	# run Makefile
	make -j$(sysctl -n hw.ncpu)

	# move Qt deps into bundle
	macdeployqt Olive.app

	# fix other deps that macdeployqt missed
	wget -c -nv https://github.com/arl/macdeployqtfix/raw/master/macdeployqtfix.py
	python2 macdeployqtfix.py Olive.app/Contents/MacOS/Olive /usr/local/Cellar/qt5/5.*/

	# move translations into bundle
	mkdir Olive.app/Contents/Translations
	mv ts/*.qm Olive.app/Contents/Translations

	# move external effects into bundle
	mkdir Olive.app/Contents/Effects
	cp effects/shaders/* Olive.app/Contents/Effects

	# distribute in zip
	zip -r Olive-$(git rev-parse --short HEAD)-macOS.zip Olive.app

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

	# generate translation files
	lrelease olive.pro

	# generate Makefile
	if [ "$ARCH" == "i386" ]; then
		# use extra compiler flags to force 32-bit build
		qmake CONFIG+=release "QMAKE_CFLAGS+=-m32" "QMAKE_CXXFLAGS+=-m32" "QMAKE_LFLAGS+=-m32" PREFIX=/usr -spec linux-g++-32
	else
		qmake CONFIG+=release PREFIX=/usr
	fi

	# run Makefile
	make -j$(nproc)

	# use `make install` on `appdir` to place files in the correct place
	make INSTALL_ROOT=appdir -j$(nproc) install ; find appdir/

	# download linuxdeployqt
	wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
	chmod a+x linuxdeployqt-continuous-x86_64.AppImage

	unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
	
	# linuxdeployqt uses this for naming the file
	export VERSION=$(git rev-parse --short HEAD)

	# use linuxdeployqt to set up dependencies
	./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -extra-plugins=imageformats/libqsvg.so -appimage

	# 64-bit linuxdeployqt can only generate a 64-bit AppImage
	# to generate a 32-bit one, we need to download and run 32-bit AppImageTool
	if [ "$ARCH" == "i386" ]; then
		rm Olive*.AppImage
		wget -c -nv "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-i686.AppImage"
		chmod a+x appimagetool-i686.AppImage
		./appimagetool-i686.AppImage "appdir" -n -g
	fi

fi
