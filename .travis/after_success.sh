#!/bin/bash

# retrieve upload tool
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh

# only create release for master
#if [ "$TRAVIS_BRANCH" != "master" ]
#then
#	export TRAVIS_EVENT_TYPE=pull_request
#fi

# Check if there's been a new commit since this build, and if so don't upload it

REMOTE=$(curl -s -N https://api.github.com/repos/olive-editor/olive/commits/master | $GREP -Po '(?<=: \")(([a-z0-9])\w+)(?=\")' -m 1)
LOCAL=$(git rev-parse HEAD)

if [ "$REMOTE" == "$LOCAL" ]
then
	echo "[INFO] This commit is still current. Uploading..."
else
	echo "[INFO] This commit is no longer current. Aborting upload."
	exit 0
fi

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

	# upload final package
	bash upload.sh Olive*.zip

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

	find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

	# upload final package
	bash upload.sh Olive*.AppImage*

elif [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	
	# upload final package
	bash upload.sh Olive*.zip
	
fi