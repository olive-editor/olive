#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

    brew install ffmpeg qt5 grep opencolorio openimageio
    export PATH="/usr/local/opt/qt/bin:/usr/local/opt/python@2/libexec/bin:$PATH"

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    sudo apt autoremove
    sudo apt-get -y install qt510base qt510multimedia qt510svg qttools5-dev libavformat-dev libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev libswresample-dev cmake libopencolorio-dev libopenimageio-dev libopenexr-dev
    source /opt/qt*/bin/qt*-env.sh

fi
