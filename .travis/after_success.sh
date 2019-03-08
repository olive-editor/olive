#!/bin/bash

# Check if there's been a new commit since this build, and if so don't upload it

GREP_PATH=grep

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
	GREP_PATH=ggrep
fi

which $GREP_PATH

# Get current repo commit from GitHub (problems arose from trying to pipe cURL directly into grep, so we buffer it through a file)
curl -s -N https://api.github.com/repos/olive-editor/olive/commits/master -o repo.txt

REMOTE=$($GREP_PATH -Po '(?<=: \")(([a-z0-9])\w+)(?=\")' -m 1 repo.txt)
LOCAL=$(git rev-parse HEAD)

if [ "$REMOTE" == "$LOCAL" ]
then
	echo "[INFO] Still current. Uploading..."

	# retrieve upload tool
	wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh

	if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

		# upload final package
		bash upload.sh Olive*.zip

	elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

		find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

		# upload final package
		bash upload.sh Olive*.AppImage*
		
	fi

else
	
	echo "[INFO] No longer current. $REMOTE vs $LOCAL - aborting upload."
	
fi
