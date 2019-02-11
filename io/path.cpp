#include "path.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>

#include "debug.h"

QString get_app_dir() {
	return QCoreApplication::applicationDirPath();
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
    // returns a list of the effects paths to search

	QList<QString> effects_paths;

    // "effects" subfolder in program folder - best for Windows
    effects_paths.append(get_app_dir() + "/effects");

    // "Effects" folder one level above the program's directory - best for Mac
    effects_paths.append(get_app_dir() + "/../Effects");

    // folder in share folder - best for Linux
	effects_paths.append(get_app_dir() + "/../share/olive-editor/effects");

    // Olive will also accept a manually provided folder with an environment variable
	QString env_path(qgetenv("OLIVE_EFFECTS_PATH"));
	if (!env_path.isEmpty()) effects_paths.append(env_path);

    return effects_paths;
}

QString get_file_hash(const QString& filename) {
	QFileInfo file_info(filename);
	QString cache_file = filename.mid(filename.lastIndexOf('/')+1) + QString::number(file_info.size()) + QString::number(file_info.lastModified().toMSecsSinceEpoch());
	return QCryptographicHash::hash(cache_file.toUtf8(), QCryptographicHash::Md5).toHex();
}

QList<QString> get_language_paths() {
	QList<QString> language_paths;

    // subfolder in program folder - best for Windows (or compiling+running from source dir)
	language_paths.append(get_app_dir() + "/ts");

    // folder one level above the program's directory - best for Mac
    language_paths.append(get_app_dir() + "/../Translations");

    // folder in share folder - best for Linux
	language_paths.append(get_app_dir() + "/../share/olive-editor/ts");

    // Olive will also accept a manually provided folder with an environment variable
	QString env_path(qgetenv("OLIVE_LANG_PATH"));
	if (!env_path.isEmpty()) language_paths.append(env_path);

	return language_paths;
}
