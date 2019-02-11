#!/bin/bash

# retrieve upload tool
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

	# upload final package
	bash upload.sh Olive*.zip

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

	find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

	# only create release for master
	if [ "$TRAVIS_BRANCH" != "master" ]
	then
		export TRAVIS_EVENT_TYPE=pull_request
	fi

	# upload final package
	bash upload.sh Olive*.AppImage*

elif [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	
	# upload final package
	bash upload.sh Olive*.zip
	
fi