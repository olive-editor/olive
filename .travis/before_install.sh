#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    # Qt 5.11
    sudo add-apt-repository ppa:beineri/opt-qt-5.11.0-xenial -y

    # FFmpeg 4.x
    sudo add-apt-repository ppa:jonathonf/ffmpeg-4 -y

    # OpenColorIO
    sudo add-apt-repository ppa:olive-editor/opencolorio -y

    # Update apt
    sudo apt-get update -qq

fi
