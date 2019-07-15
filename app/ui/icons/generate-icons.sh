#!/bin/sh

# Olive - Non-Linear Video Editor
# Copyright (C) 2019 Olive Team
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
  echo "Usage: $0 icon-pack-name [options]"
  echo
  echo "Example: $0 olive-dark"
  echo
  echo "Options:"
  echo "    -n    Only generate QRC file"
  echo
  exit 1
fi

PACKNAME=$1
SVGDIR=$1/svg
PNGDIR=$1
QRCFILE=$1/$1.qrc

ONLYQRC=0

if [ "$2" == "-n" ]
then
  ONLYQRC=1
fi

truncate -s 0 $QRCFILE

echo "<RCC>" >> $QRCFILE
echo "  <qresource prefix=\"/icons/$PACKNAME\">" >> $QRCFILE

OutputPng() {
  echo Creating $2...

  echo "    <file>$(basename $2)</file>" >> $QRCFILE

  if [ $ONLYQRC -eq 0 ]
  then
    inkscape -z -e $(pwd)/$2 -w $s -h $s $1
  fi
}

for f in $SVGDIR/*.svg
do
  FNBASE=$(basename -s .svg $f)

  for s in "${OUTPUT_SIZES[@]}"
  do
    OutputPng $f $PNGDIR/$FNBASE.$s.png
  done
done

echo "  </qresource>" >> $QRCFILE
echo "</RCC>" >> $QRCFILE
