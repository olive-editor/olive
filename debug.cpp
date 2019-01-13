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
		debug_info.prepend(QString("<b>[DEBUG]</b> %1 (%2:%3, %4)<br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtInfoMsg:
		fprintf(stderr, "[INFO] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[INFO] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.prepend(QString("<b>[INFO]</b> %1 (%2:%3, %4)<br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtWarningMsg:
		fprintf(stderr, "[WARNING] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[WARNING] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.prepend(QString("<font color='yellow'><b>[WARNING]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "[ERROR] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[ERROR] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.prepend(QString("<font color='red'><b>[ERROR]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtFatalMsg:
		fprintf(stderr, "[FATAL] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		if (debug_file.isOpen()) debug_stream << QString("[FATAL] %1 (%2:%3, %4)\n").arg(localMsg.constData(), context.file, QString::number(context.line), context.function);
		debug_info.prepend(QString("<font color='red'><b>[FATAL]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		abort();
	}
	if (debug_dialog != nullptr && debug_dialog->isVisible()) {
		QMetaObject::invokeMethod(debug_dialog, "update_log", Qt::QueuedConnection);
	}
	debug_mutex.unlock();
}

const QString &get_debug_str() {
	return debug_info;
}
