#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then

    export PATH="/usr/local/opt/qt/bin:/usr/local/opt/python@2/libexec/bin:$PATH"

elif [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

    sudo apt-get -y -o Dpkg::Options::="--force-overwrite" install qt511base qt511multimedia qt511svg qt511tools libavformat-dev libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev libswresample-dev libopencolorio-dev libopenimageio-dev libgl1-mesa-dev
    source /opt/qt*/bin/qt*-env.sh

    # Acquire latest cmake (apt somehow gets the wrong version?)
    wget -c https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-Linux-x86_64.sh -O cmake.sh
    chmod +x cmake.sh
    ./cmake.sh --skip-license --prefix=cmake --exclude-dir
    export PATH=$PWD/cmake/bin:$PATH

fi
