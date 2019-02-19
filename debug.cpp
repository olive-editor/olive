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

void close_debug_file()
{
    if (debug_file.isOpen()) {
        debug_file.close();
    }
}

void debug_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    debug_mutex.lock();
    const QByteArray localMsg = msg.toLocal8Bit();
    const QDateTime now = QDateTime::currentDateTime();
    const QByteArray timeRepr(now.toString(Qt::ISODate).toLocal8Bit());
    QString msgTag;
    QString fontColor;
    switch (type) {
    case QtDebugMsg:
        msgTag = "DEBUG";
        fontColor = "grey";
        break;
    case QtInfoMsg:
        msgTag = "INFO";
        fontColor = "blue";
        break;
    case QtWarningMsg:
        msgTag = "WARNING";
        fontColor = "yellow";
        break;
    case QtCriticalMsg:
        msgTag = "ERROR";
        fontColor = "red";
        break;
    case QtFatalMsg:
        msgTag = "FATAL";
        fontColor = "red";
        break;
    default:
        fprintf(stderr, "Unknown debug msg type");
        fflush(stderr);
        break;
    }//switch

    fprintf(stderr, "%s [%s] %s (%s:%u, %s)\n", timeRepr.data(), msgTag.toLocal8Bit().constData(), localMsg.data(),
            context.file, context.line, context.function);
    if (debug_file.isOpen()) {
        debug_stream << QString("[%1] %2 (%3:%4, %5)\n")
                        .arg(msgTag, localMsg, context.file, QString::number(context.line), context.function);
    }
    debug_info.prepend(QString("<font color='%1'><b>[%2]</b> %3 (%4:%5, %6)</font><br>")
                       .arg(fontColor, msgTag, localMsg, context.file, QString::number(context.line), context.function));
    fflush(stderr);
    if (olive::DebugDialog != nullptr && olive::DebugDialog->isVisible()) {
        QMetaObject::invokeMethod(olive::DebugDialog, "update_log", Qt::QueuedConnection);
    }
    debug_mutex.unlock();
}

const QString &get_debug_str()
{
    return debug_info;
}
