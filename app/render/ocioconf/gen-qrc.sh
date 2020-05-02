#!/bin/sh
ourbasename=$(basename "$0")

rm ocioconf.qrc
echo "<RCC>" >> ocioconf.qrc
echo "  <qresource prefix=\"/ocioconf\">" >> ocioconf.qrc

for f in $(find * -type f)
do
    if [ "$f" != "CMakeLists.txt" ] && [ "$f" != "ocioconf.qrc" ] && [ "$f" != "$ourbasename" ]
    then
        echo "    <file>$f</file>" >> ocioconf.qrc
    fi
done

echo "  </qresource>" >> ocioconf.qrc
echo "</RCC>" >> ocioconf.qrc
