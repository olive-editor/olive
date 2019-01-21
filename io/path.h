#ifndef PATH_H
#define PATH_H

#include <QString>

QString get_app_dir();
QString get_data_path();
QString get_config_path();
QList<QString> get_effects_paths();

#endif // PATH_H
