#include "debug.h"

#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

#include "dialogs/debugdialog.h"

QString debug_info;

void debug_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	QByteArray localMsg = msg.toLocal8Bit();
	switch (type) {
	case QtDebugMsg:
		fprintf(stderr, "[DEBUG] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		debug_info.prepend(QString("<b>[DEBUG]</b> %1 (%2:%3, %4)<br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtInfoMsg:
		fprintf(stderr, "[INFO] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		debug_info.prepend(QString("<b>[INFO]</b> %1 (%2:%3, %4)<br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtWarningMsg:
		fprintf(stderr, "[WARNING] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		debug_info.prepend(QString("<font color='yellow'><b>[WARNING]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "[ERROR] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		debug_info.prepend(QString("<font color='red'><b>[ERROR]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		break;
	case QtFatalMsg:
		fprintf(stderr, "[FATAL] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		debug_info.prepend(QString("<font color='red'><b>[FATAL]</b> %1 (%2:%3, %4)</font><br>").arg(localMsg.constData(), context.file, QString::number(context.line), context.function));
		fflush(stderr);
		abort();
	}
	if (debug_dialog->isVisible()) {
		QMetaObject::invokeMethod(debug_dialog, "update_log", Qt::QueuedConnection);
	}
}

const QString &get_debug_str() {
	return debug_info;
}
