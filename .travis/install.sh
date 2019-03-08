#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

	brew install ffmpeg qt5 grep
	export PATH="/usr/local/opt/qt/bin:/usr/local/opt/python@2/libexec/bin:$PATH"

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

	if [ "$ARCH" == "x86_64" ]; then
		sudo apt-get -y install qt59base qt59multimedia qt59svg libavformat-dev libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev libswresample-dev frei0r-plugins-dev frei0r-plugins fuse
	fi

	if [ "$ARCH" == "i386" ]; then
		sudo apt-get -y install gcc-multilib g++-multilib qt59base:i386 qt59multimedia:i386 qt59svg:i386 libavformat-dev:i386 libavcodec-dev:i386 libavfilter-dev:i386 libavutil-dev:i386 libswscale-dev:i386 libswresample-dev:i386 frei0r-plugins-dev:i386 frei0r-plugins:i386 pkg-config:i386 libgl1-mesa-dev:i386 fuse:i386
	fi
	source /opt/qt*/bin/qt*-env.sh

fi

