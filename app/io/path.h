/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

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
