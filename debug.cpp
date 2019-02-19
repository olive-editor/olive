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

#include "debug.h"

#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QMutex>

#include "dialogs/debugdialog.h"

QString debug_info;
QMutex debug_mutex;
QFile debug_file;
QTextStream debug_stream;

void open_debug_file() {
	QDir debug_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	debug_dir.mkpath(".");
	if (debug_dir.exists()) {
		debug_file.setFileName(debug_dir.path() + "/debug_log");
		if (debug_file.open(QFile::WriteOnly)) {
			debug_stream.setDevice(&debug_file);
		} else {
			qWarning() << "Couldn't open debug log file, debug log will not be saved";
		}
	}
}

void close_debug_file() {
	if (debug_file.isOpen()) debug_file.close();
}

void debug_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	debug_mutex.lock();
	QByteArray localMsg = msg.toLocal8Bit();
	switch (type) {
	case QtDebugMsg:
		fprintf(stderr, "[DEBUG] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[DEBUG] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.append(QString("<b>[DEBUG]</b> %1 (%2:%3, %4)<br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtInfoMsg:
		fprintf(stderr, "[INFO] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[INFO] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.append(QString("<b>[INFO]</b> %1 (%2:%3, %4)<br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtWarningMsg:
		fprintf(stderr, "[WARNING] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[WARNING] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.append(QString("<font color='yellow'><b>[WARNING]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "[ERROR] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[ERROR] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.append(QString("<font color='red'><b>[ERROR]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtFatalMsg:
		fprintf(stderr, "[FATAL] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[FATAL] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.append(QString("<font color='red'><b>[FATAL]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
//		abort();
	}
    if (Olive::DebugDialog != nullptr && Olive::DebugDialog->isVisible()) {
        QMetaObject::invokeMethod(Olive::DebugDialog, "update_log", Qt::QueuedConnection);
	}
	debug_mutex.unlock();
}

const QString &get_debug_str() {
	return debug_info;
}
