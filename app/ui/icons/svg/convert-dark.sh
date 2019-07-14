#!/bin/sh

for f in *.svg
do
  FNBASE=$(basename -s .svg $f)

  sed -e "s/#ffffff/#000000/" $f > $FNBASE-dark.svg
done