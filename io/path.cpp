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
#include "path.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QCoreApplication>
#include "debug.h"

QString real_app_dir;

QString get_app_dir() {
	if (real_app_dir.isEmpty()) {
		QString app_path = QCoreApplication::applicationFilePath();
		real_app_dir = app_path.left(app_path.lastIndexOf('/'));
	}
	return real_app_dir;
}

QString get_data_path() {
	QString app_dir = get_app_dir();
	if (QFileInfo::exists(app_dir + "/portable")) {
		return app_dir;
	} else {
		return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	}
}

QString get_config_path() {
	QString app_dir = get_app_dir();
	if (QFileInfo::exists(app_dir + "/portable")) {
		return app_dir;
	} else {
		return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
	}
}
