#include "path.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QCoreApplication>

QString get_data_path() {
    QString app_path = QCoreApplication::applicationFilePath();
    QString app_dir = app_path.left(app_path.lastIndexOf('/'));

    if (QFileInfo::exists(app_dir + "/portable")) {
        return app_dir;
    } else {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
}
