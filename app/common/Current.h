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
 *
 */

#ifndef CURRENT_H
#define CURRENT_H
#include "pluginSupport/OliveHost.h"
#include "render/videoparams.h"
#include "render/job/pluginjob.h"

class Current {
public:
	static Current &getInstance()
	{
		return current;
	}
	olive::VideoParams &currentVideoParams()
	{
		return currentVideoParams_;
	}
	olive::AudioParams &currentAudioParams()
	{
		return currentAudioParams_;
	}
	void setCurrentVideoParams(olive::VideoParams &params)
	{
		currentVideoParams_ = params;
	}
	void setCurrentAudioParams(olive::AudioParams &params)
	{
		currentAudioParams_ = params;
	}
	void setCurrentVideoParams(olive::VideoParams &&params)
	{
		currentVideoParams_ = params;
	}
	void setCurrentAudioParams(olive::AudioParams &&params)
	{
		currentAudioParams_ = params;
	}
	bool interactive()
	{
		return true;
	}

	std::shared_ptr<olive::plugin::OliveHost> pluginHost()
	{
		return myHost;
	}

	void setPluginHost(std::shared_ptr<olive::plugin::OliveHost> host)
	{
		myHost = host;
	}

	std::shared_ptr<OFX::Host::ImageEffect::PluginCache> pluginCache()
	{
		return plugin_cache_;
	}

	void
	setPluginCache(std::shared_ptr<OFX::Host::ImageEffect::PluginCache> cache)
	{
		plugin_cache_ = cache;
	}

private:
	static Current current;
	olive::VideoParams currentVideoParams_;
	olive::AudioParams currentAudioParams_;
	std::shared_ptr<olive::plugin::OliveHost> myHost;
	std::shared_ptr<OFX::Host::ImageEffect::PluginCache> plugin_cache_;
};

#endif //CURRENT_H
