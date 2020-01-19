#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    # Qt 5.10.1 (may have to source newer version at some point)
    sudo add-apt-repository ppa:beineri/opt-qt-5.10.1-trusty -y

    # FFmpeg 4.x
    sudo add-apt-repository ppa:jonathonf/ffmpeg-4 -y

    # OpenImageIO
    sudo add-apt-repository ppa:olive-editor/openimageio -y

    # OpenColorIO
    sudo add-apt-repository ppa:olive-editor/opencolorio -y

    # Update apt
    sudo apt-get update -qq

fi
