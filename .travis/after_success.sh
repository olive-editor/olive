#!/bin/bash

# Check if there's been a new commit since this build, and if so don't upload it

GREP_PATH=grep

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    GREP_PATH=ggrep
fi

if [ "$TRAVIS_TAG" != "" ]; then
    
fi

# Get current repo commit from GitHub (problems arose from trying to pipe cURL directly into grep, so we buffer it through a file)
REMOTE=$(curl -H "Authorization: token $GITHUB_TOKEN" https://api.github.com/repos/olive-editor/olive/commits/master | $GREP_PATH -Po '(?<=: \")(([a-z0-9])\w+)(?=\")' -m 1 --)
LOCAL=$(git rev-parse HEAD)

if [ "$TRAVIS_TAG" != "" || "$REMOTE" == "$LOCAL" ]
then
    echo "[INFO] Still current. Uploading..."

    # Retrieve upload tool
    wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh

    if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

        upload final package
        bash upload.sh Olive*.zip

    elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

        find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

        # Upload final package
        bash upload.sh Olive*.AppImage*
        
    fi

else
    
    echo "[INFO] No longer current. $REMOTE vs $LOCAL - aborting upload."
    
fi
