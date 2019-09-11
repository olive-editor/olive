#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    # Qt 5.9.3 - FIXME: Consider finding newer version
    sudo add-apt-repository ppa:beineri/opt-qt593-trusty -y

    # FFmpeg 4.x (uses libs from 3.x repo)
    sudo add-apt-repository ppa:jonathonf/ffmpeg-3 -y
    sudo add-apt-repository ppa:jonathonf/ffmpeg-4 -y

    # OpenImageIO
    sudo add-apt-repository ppa:olive-editor/openimageio -y

    # Update apt
    sudo apt-get update -qq

fi
