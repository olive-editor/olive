/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "crc32.h"

#include <QFile>

Crc32::Crc32()
{
    quint32 crc;

    // initialize CRC table
    for (int i = 0; i < 256; i++)
    {
        crc = i;
        for (int j = 0; j < 8; j++)
            crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;

        crc_table[i] = crc;
    }
}

quint32 Crc32::calculateFromFile(QString filename)
{
    quint32 crc;
    QFile file;

    char buffer[16000];
    int len, i;

    crc = 0xFFFFFFFFUL;

    file.setFileName(filename);
    if (file.open(QIODevice::ReadOnly))
    {
        while (!file.atEnd())
        {
            len = file.read(buffer, 16000);
            for (i = 0; i < len; i++)
                crc = crc_table[(crc ^ buffer[i]) & 0xFF] ^ (crc >> 8);
        }

        file.close();
    }

    return crc ^ 0xFFFFFFFFUL;
}

void Crc32::initInstance(int i)
{
    instances[i] = 0xFFFFFFFFUL;
}

void Crc32::pushData(int i, char *data, int len)
{
    quint32 crc = instances[i];
    if (crc)
    {
        for (int j = 0; j < len; j++)
            crc = crc_table[(crc ^ data[j]) & 0xFF] ^ (crc >> 8);

        instances[i] = crc;
    }
}

quint32 Crc32::releaseInstance(int i)
{
    quint32 crc32 = instances[i];
    if (crc32) {
        instances.remove(i);
        return crc32 ^ 0xFFFFFFFFUL;
    }
    else {
        return 0;
    }
}
