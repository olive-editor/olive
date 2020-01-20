#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

    export PATH="/usr/local/opt/qt/bin:/usr/local/opt/python@2/libexec/bin:$PATH"

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    sudo apt-get -y -o Dpkg::Options::="--force-overwrite" install qt511base qt511multimedia qt511svg qt511tools libavformat-dev libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev libswresample-dev cmake libopencolorio-dev libopenimageio2-dev libgl1-mesa-dev
    source /opt/qt*/bin/qt*-env.sh

fi
