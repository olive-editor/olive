/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "node/project.h"
#include "ofxhImageEffect.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <ofxhPluginCache.h>
#include <ofxhBinary.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include "OliveHost.h"

#include "OlivePluginInstance.h"
#include "common/Current.h"
#include "ofxMessage.h"
#include "version.h"
#include <QMessageBox>
using namespace OFX::Host;
using namespace olive::plugin;

namespace olive
{
namespace plugin
{
class PluginNode;
}
}

namespace
{
void AddPluginPath(OFX::Host::PluginCache *cache, const QString &path,
				   bool recurse = true)
{
	if (!cache || path.isEmpty()) {
		return;
	}
	QDir dir(path);
	if (!dir.exists()) {
		return;
	}
	cache->addFileToPath(dir.canonicalPath().toStdString(), recurse);
}

void AddPluginPathsFromEnv(OFX::Host::PluginCache *cache, const char *env_var)
{
	QString raw = qEnvironmentVariable(env_var);
	if (raw.isEmpty()) {
		return;
	}
	const QChar separator = QDir::listSeparator();
	const QStringList paths = raw.split(separator, Qt::SkipEmptyParts);
	for (const QString &path : paths) {
		AddPluginPath(cache, path);
	}
}
}

void olive::plugin::loadPlugins(QString path)
{
	std::shared_ptr<OliveHost> host = Current::getInstance().pluginHost();
	std::shared_ptr<ImageEffect::PluginCache> imageEffectPluginCache =
		Current::getInstance().pluginCache();

	if (!host || !imageEffectPluginCache) {
		host = std::make_shared<OliveHost>();
		Current::getInstance().setPluginHost(host);

		imageEffectPluginCache =
			std::make_shared<ImageEffect::PluginCache>(*host);
		Current::getInstance().setPluginCache(imageEffectPluginCache);

		imageEffectPluginCache->registerInCache(
			*OFX::Host::PluginCache::getPluginCache());
	}
	OFX::Host::PluginCache *cache = OFX::Host::PluginCache::getPluginCache();
	cache->setPluginHostPath("Olive");

	const QString home_path = QDir::homePath();
	AddPluginPath(cache, QDir(home_path).filePath(".OFX/Plugins"));
	AddPluginPath(cache, QDir(home_path).filePath(".local/share/OFX/Plugins"));
	AddPluginPath(cache,
				  QDir(home_path).filePath(".local/share/olive/ofx/Plugins"));

	const QString app_dir = QCoreApplication::applicationDirPath();
	AddPluginPath(cache, QDir(app_dir).filePath("../OFX/Plugins"));
	AddPluginPath(cache, QDir(app_dir).filePath("../share/olive/ofx/Plugins"));
	AddPluginPath(cache, QDir(app_dir).filePath("../lib/olive/ofx/Plugins"));

	AddPluginPathsFromEnv(cache, "OLIVE_OFX_PLUGIN_PATH");
	AddPluginPathsFromEnv(cache, "OLIVE_PLUGIN_PATH");

	if (!path.isEmpty()) {
		AddPluginPath(cache, path, true);
	}
	cache->scanPluginFiles();
}
OliveHost::OliveHost()
{
	// Identify the host to plugins; HostSupport seeds these with "UNKNOWN".
	_properties.setStringProperty(kOfxPropName, "Oak Video Editor");
	_properties.setStringProperty(kOfxPropLabel, "Oak Video Editor");
	_properties.setStringProperty(kOfxPropVersionLabel,
								  olive::kAppVersion.toStdString());

	// Numeric version for plugins that query kOfxPropVersion directly.
	const QStringList version_parts =
		olive::kAppVersion.section(QLatin1Char('-'), 0, 0)
			.split(QLatin1Char('.'));
	_properties.setIntProperty(kOfxPropVersion,
							   version_parts.value(0).toInt(), 0);
	_properties.setIntProperty(kOfxPropVersion,
							   version_parts.value(1).toInt(), 1);
	_properties.setIntProperty(kOfxPropVersion,
							   version_parts.value(2).toInt(), 2);
}

OliveHost::~OliveHost()
{
}

void OliveHost::destroyInstance(OFX::Host::ImageEffect::Instance *instance)
{
	if (!instance) {
		return;
	}
	for (auto it = instances_.begin(); it != instances_.end(); ++it) {
		if (it->get() == instance) {
			instances_.erase(it);
			break;
		}
	}
}
std::shared_ptr<OFX::Host::ImageEffect::Descriptor>
OliveHost::makeDescriptor(ImageEffect::ImageEffectPlugin *plugin)
{
	std::shared_ptr<OFX::Host::ImageEffect::Descriptor> desc =
		std::make_shared<ImageEffect::Descriptor>(plugin);
	descriptors_.append(std::shared_ptr<ImageEffect::Descriptor>(desc));
	return desc;
}
std::shared_ptr<OFX::Host::ImageEffect::Descriptor>
OliveHost::makeDescriptor(const ImageEffect::Descriptor &rootContext,
						  ImageEffect::ImageEffectPlugin *plugin)
{
	std::shared_ptr<OFX::Host::ImageEffect::Descriptor> desc =
		std::make_shared<ImageEffect::Descriptor>(rootContext, plugin);
	descriptors_.append(std::shared_ptr<ImageEffect::Descriptor>(desc));
	return desc;
}
std::shared_ptr<OFX::Host::ImageEffect::Descriptor>
OliveHost::makeDescriptor(const std::string &bundlePath,
						  ImageEffect::ImageEffectPlugin *plugin)
{
	std::shared_ptr<OFX::Host::ImageEffect::Descriptor> desc =
		std::make_shared<ImageEffect::Descriptor>(bundlePath, plugin);
	descriptors_.append(std::shared_ptr<ImageEffect::Descriptor>(desc));
	return desc;
}

ImageEffect::Instance *
OliveHost::newInstance(void *clientData, ImageEffect::ImageEffectPlugin *plugin,
					   ImageEffect::Descriptor &desc,
					   const std::string &context)
{
	auto *instance = new OlivePluginInstance(
		plugin, desc, context, Current::getInstance().interactive());
	if (clientData) {
		auto *node = static_cast<PluginNode *>(clientData);
		instance->setNode(
			std::shared_ptr<PluginNode>(node, [](PluginNode *) {}));
	}
	instances_.append(std::shared_ptr<OlivePluginInstance>(instance));
	return instance;
};
OfxStatus olive::plugin::OliveHost::vmessage(const char *type, const char *id,
											 const char *format, va_list args)
{
	if (!type || !format) {
		return kOfxStatFailed;
	}

	char buffer[1024];
	buffer[0] = '\0';
	vsnprintf(buffer, sizeof(buffer), format, args);
	QString message(buffer);

	auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
	// A modal dialog would hang a headless (offscreen) session, so log to
	// stderr instead of showing one.
	if (!app || QGuiApplication::platformName() == QLatin1String("offscreen")) {
		qWarning().noquote() << "OFX message:" << type << message;
		if (strcmp(type, kOfxMessageQuestion) == 0) {
			return kOfxStatReplyNo;
		}
		return kOfxStatOK;
	}

	if (strcmp(type, kOfxMessageQuestion) == 0) {
		auto ret = QMessageBox::question(nullptr, "", message, QMessageBox::Ok,
										 QMessageBox::Cancel);
		return (ret == QMessageBox::Ok) ? kOfxStatReplyYes : kOfxStatReplyNo;
	}

	if (strcmp(type, kOfxMessageError) == 0) {
		QMessageBox::critical(nullptr, "", message);
	} else if (strcmp(type, kOfxMessageWarning) == 0) {
		QMessageBox::warning(nullptr, "", message);
	} else {
		QMessageBox::information(nullptr, "", message);
	}

	return kOfxStatOK;
}
// TODO: Persistent messages shouldn't use pop-up window.
OfxStatus olive::plugin::OliveHost::setPersistentMessage(const char *type,
														 const char *id,
														 const char *format,
														 va_list args)
{
	if (!type || !format) {
		return kOfxStatFailed;
	}

	char buffer[1024];
	buffer[0] = '\0';
	vsnprintf(buffer, sizeof(buffer), format, args);
	QString message(buffer);

	// A modal dialog would hang a headless (offscreen) session, so log to
	// stderr instead of showing one.
	const bool headless =
		QGuiApplication::platformName() == QLatin1String("offscreen");

	if (strcmp(type, kOfxMessageError) == 0) {
		persistent_messages_.append({ HostMessageType::Error, message });
		if (headless) {
			qWarning().noquote() << "OFX error:" << message;
		} else {
			QMessageBox::critical(nullptr, "", message);
		}
	} else if (strcmp(type, kOfxMessageWarning) == 0) {
		persistent_messages_.append({ HostMessageType::Warning, message });
		if (headless) {
			qWarning().noquote() << "OFX warning:" << message;
		} else {
			QMessageBox::warning(nullptr, "", message);
		}
	} else if (strcmp(type, kOfxMessageMessage) == 0) {
		persistent_messages_.append({ HostMessageType::Message, message });
		if (headless) {
			qWarning().noquote() << "OFX message:" << message;
		} else {
			QMessageBox::information(nullptr, "", message);
		}
	} else {
		return kOfxStatFailed;
	}

	return kOfxStatOK;
}

OfxStatus olive::plugin::OliveHost::clearPersistentMessage()
{
	persistent_messages_.clear();
	return kOfxStatOK;
}
