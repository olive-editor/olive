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
#ifndef CRC32_H
#define CRC32_H

/*
 * +-------------------------------------------------------------+
 * | Taken from github.com/nusov/qt-crc32 used under MIT license |
 * |															 |
 * | Copyright (c) Alexander Nusov 2015							 |
 * +-------------------------------------------------------------+
 */

#include <QtCore>
#include <QString>
#include <QMap>

class Crc32
{
private:
    quint32 crc_table[256];
    QMap<int, quint32> instances;

public:
    Crc32();

    quint32 calculateFromFile(QString filename);

    void initInstance(int i);
    void pushData(int i, char *data, int len);
    quint32 releaseInstance(int i);
};

#endif // CRC32_H
