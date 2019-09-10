#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    # install apt repos necessary for Olive
    sudo add-apt-repository ppa:beineri/opt-qt593-trusty -y
    sudo add-apt-repository ppa:jonathonf/ffmpeg-3 -y
    sudo add-apt-repository ppa:jonathonf/ffmpeg-4 -y
    sudo apt-get update -qq

fi
