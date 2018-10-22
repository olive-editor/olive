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
