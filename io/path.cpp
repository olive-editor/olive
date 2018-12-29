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
