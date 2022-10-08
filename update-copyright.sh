#!/bin/sh

if [ "$#" -ne 2 ]
then
  echo "Usage: $0 [old-year] [new-year]"
  exit 1
fi

find . -type f -exec sed -i "s/$1 Olive Team/$2 Olive Team/g" {} \;
