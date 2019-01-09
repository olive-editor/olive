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
#include "debug.h"

#include <QFile>
#include <QDateTime>
#include <QStandardPaths>

#ifndef QT_DEBUG
QFile debug_file;
QDebug debug_out(&debug_file);
#endif

void setup_debug() {
#ifndef QT_DEBUG
	debug_file.setFileName(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/debug_log");
	if (debug_file.open(QFile::WriteOnly)) {
		QString debug_intro = "Olive Session " + QString::number(QDateTime::currentMSecsSinceEpoch());
		debug_file.write(debug_intro.toUtf8());
	} else {
		debug_out = QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug();
	}
#endif
}

void close_debug() {
#ifndef QT_DEBUG
	if (debug_file.isOpen()) {
		debug_file.putChar(10);
		debug_file.putChar(10);
		debug_file.close();
	}
#endif
}
