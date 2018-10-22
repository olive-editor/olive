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
