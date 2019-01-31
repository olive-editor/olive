#include "path.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>

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

QList<QString> get_effects_paths() {
	QList<QString> effects_paths;
	effects_paths.append(get_app_dir() + "/effects");
	effects_paths.append(get_app_dir() + "/../share/olive-editor/effects");
	QString env_path(qgetenv("OLIVE_EFFECTS_PATH"));
	if (!env_path.isEmpty()) effects_paths.append(env_path);
	return effects_paths;
}

QString get_file_hash(const QString& filename) {
	QFileInfo file_info(filename);
	QString cache_file = filename.mid(filename.lastIndexOf('/')+1) + QString::number(file_info.size()) + QString::number(file_info.lastModified().toMSecsSinceEpoch());
	return QCryptographicHash::hash(cache_file.toUtf8(), QCryptographicHash::Md5).toHex();
}
