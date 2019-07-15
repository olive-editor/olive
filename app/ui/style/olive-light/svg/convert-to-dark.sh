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

# convert-to-dark.sh
#
# A simple script that uses sed/regex to convert any use of white to dark grey.
# Allows for easy conversion of white icons for a dark theme to dark icons for
# a light theme.

if [ $# -ne 2 ]
then
  echo Usage: $0 source-path destination-path
  echo
  echo Example: $0 ../../olive-dark/svg .
  exit 1
fi

for f in $1/*.svg
do
  FNBASE=$(basename -s .svg $f)

  sed -e "s/#ffffff/#202020/g" $f > $FNBASE.svg
done
