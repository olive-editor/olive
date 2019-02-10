#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]
then
	wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
	upload.sh Olive*.zip
elif [[ "$TRAVIS_OS_NAME" == "linux" ]]
then
	find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
	# curl --upload-file Olive*.AppImage https://transfer.sh/Olive-git.$(git rev-parse --short HEAD)-$ARCH.AppImage
	wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
	# only create release for master
	if [ "$TRAVIS_BRANCH" != "master" ]
	then
		export TRAVIS_EVENT_TYPE=pull_request
	fi
	upload.sh Olive*.AppImage*
fi