#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include <QString>

QString GetUniqueFileIdentifier(const QString& filename);

QString GetMediaIndexLocation();

QString GetMediaIndexFilename(const QString& filename);

#endif // FILEFUNCTIONS_H
