#!/bin/sh

# Olive - Non-Linear Video Editor
# Copyright (C) 2021 Olive Team
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# genicons.sh
#
# A simple script that uses Inkscape to generate multiple PNG sizes from SVGs.
# It also generates a Qt resource file including all of them for usage in
# Olive.
#
# While Qt (and therefore Olive) can load SVG files into icons during runtime,
# this is slow and lags startup. This script is intended to mitigate that, and
# while it adds an extra step if an SVG is modified, it immensely benefits
# the end user.

OUTPUT_SIZES=( 16 32 64 128 )

if [ $# -lt 1 ]
then
  echo "Generates sized PNG icons from SVG files"
  echo "Usage: $0 icon-pack-name"
  echo
  echo "Example: $0 olive-dark"
  echo
  exit 1
fi

PACKNAME=$1
SVGDIR=$1/svg
PNGDIR=$1/png

mkdir -p $PNGDIR

OutputPng() {
  inkscape --export-filename $2.png --export-width $s --export-height $s $1
  convert $2.png -alpha set -background none -channel A -evaluate multiply 0.25 +channel $2.disabled.png
}

for f in $SVGDIR/*.svg
do
  FNBASE=$(basename -s .svg $f)

  for s in "${OUTPUT_SIZES[@]}"
  do
    OutputPng $f $PNGDIR/$FNBASE.$s
  done
done
