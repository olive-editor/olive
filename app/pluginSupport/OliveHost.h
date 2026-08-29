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
#ifndef OLIVE_HOST_H
#define OLIVE_HOST_H
#include "node/plugins/Plugin.h"
#include "ofxhHost.h"
#include "ofxhImageEffectAPI.h"
#include "ofxCore.h"
#include "ofxhImageEffect.h"

#include <QVariant>
#include <cstdint>
#include <QString>
#include <QMap>
#include <any>
#include <list>
#include <memory>
#include <qlist.h>
namespace olive
{
namespace plugin
{
enum class HostMessageType { Error, Warning, Message };
struct HostPersistentMessage {
	HostMessageType type;
	QString message;
};

void loadPlugins(QString path);
class OliveHost : public OFX::Host::ImageEffect::Host {
public:
	OliveHost();
	~OliveHost() override;
	void destroyInstance(OFX::Host::ImageEffect::Instance *instance);

	bool pluginSupported(OFX::Host::ImageEffect::ImageEffectPlugin *plugin,
						 std::string &reason) const override
	{
		if (!plugin) {
			reason = "null plugin";
			return false;
		}
		if (plugin->getContexts().empty()) {
			reason = "no supported contexts (describe failed)";
			return false;
		}
		return true;
	};

	OFX::Host::ImageEffect::Instance *
	newInstance(void *clientData,
				OFX::Host::ImageEffect::ImageEffectPlugin *plugin,
				OFX::Host::ImageEffect::Descriptor &desc,
				const std::string &context) override;

	std::shared_ptr<OFX::Host::ImageEffect::Descriptor>
	makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin *plugin) override;

	std::shared_ptr<OFX::Host::ImageEffect::Descriptor>
	makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
				   OFX::Host::ImageEffect::ImageEffectPlugin *plugin) override;

	std::shared_ptr<OFX::Host::ImageEffect::Descriptor>
	makeDescriptor(const std::string &bundlePath,
				   OFX::Host::ImageEffect::ImageEffectPlugin *plugin) override;
	/// vmessage
	virtual OfxStatus vmessage(const char *type, const char *id,
							   const char *format, va_list args);

	/// vmessage
	virtual OfxStatus setPersistentMessage(const char *type, const char *id,
										   const char *format, va_list args);
	/// vmessage
	virtual OfxStatus clearPersistentMessage();

#ifdef OFX_SUPPORTS_OPENGLRENDER
	/// @see OfxImageEffectOpenGLRenderSuiteV1.flushResources()
	virtual OfxStatus flushOpenGLResources() const
	{
		return kOfxStatFailed;
	};
#endif
private:
	QList<std::shared_ptr<OFX::Host::ImageEffect::Descriptor>> descriptors_;
	QList<std::shared_ptr<OFX::Host::ImageEffect::Instance>> instances_;
	QList<HostPersistentMessage> persistent_messages_;
};
}
}
#endif
