#!/bin/bash

#if [[ "$TRAVIS_OS_NAME" == "osx" ]]
#then
	# do nothing
#elif [[ "$TRAVIS_OS_NAME" == "linux" ]]
if [[ "$TRAVIS_OS_NAME" == "linux" ]]
then
	sudo add-apt-repository ppa:beineri/opt-qt593-trusty -y
	sudo add-apt-repository ppa:jonathonf/ffmpeg-3 -y
	sudo apt-get update -qq
fi