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

if [ $# -ne 2 ]
then
  echo Usage: $0 path-to-svgs qrc-file
  echo
  echo Example: $0 svg icons.qrc
  exit 1
fi

SVGDIR=$1
QRCFILE=$2

truncate -s 0 $QRCFILE

echo "<RCC>" >> $QRCFILE
echo "  <qresource prefix=\"/icons\">" >> $QRCFILE

for f in $1/*.svg
do
  FNBASE=$(basename -s .svg $f)

  for s in "${OUTPUT_SIZES[@]}"
  do
    OUTPUTFN=$FNBASE.$s.png

    echo Creating $OUTPUTFN...

    echo "    <file>$OUTPUTFN</file>" >> $QRCFILE
    
    inkscape -z -e $(pwd)/$OUTPUTFN -w $s -h $s $f
  done
done

echo "  </qresource>" >> $QRCFILE
echo "</RCC>" >> $QRCFILE
