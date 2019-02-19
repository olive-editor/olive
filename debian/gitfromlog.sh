# A simple script to extract the Git hash from an auto-generated debian/changelog

grep -Po '(?<=-)(([a-z0-9])\w+)(?=\+)' -m 1 $1