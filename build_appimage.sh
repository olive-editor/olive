#!/bin/sh

TAG=2022.3

git submodule update --init

docker run --rm -it -v $(pwd):/opt/olive --entrypoint=/opt/olive/docker/scripts/build_appimage.sh olivevideoeditor/ci-olive:$TAG
