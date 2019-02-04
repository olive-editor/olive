#ifndef PATH_H
#define PATH_H

#include <QString>

QString get_app_dir();
QString get_data_path();
QString get_config_path();
QList<QString> get_effects_paths();
QList<QString> get_language_paths();

// generate hash algorithm used to uniquely identify files
QString get_file_hash(const QString& filename);

#endif // PATH_H
