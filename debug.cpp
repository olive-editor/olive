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
	debug_file.setFileName(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/debug_log");
	if (debug_file.open(QFile::WriteOnly)) {
		QString debug_intro = "Olive Session " + QString::number(QDateTime::currentMSecsSinceEpoch());
		debug_file.write(debug_intro.toLatin1());
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
