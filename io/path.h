#ifndef PATH_H
#define PATH_H

#include <QString>
#include <QDir>

QString get_app_path();
QString get_data_path();
QDir get_data_dir();
QString get_config_path();
QDir get_config_dir();
QList<QString> get_effects_paths();
QList<QString> get_language_paths();

// generate hash algorithm used to uniquely identify files
QString get_file_hash(const QString& filename);

#endif // PATH_H
