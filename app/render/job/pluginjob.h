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

#ifndef PLUGINJOB_H
#define PLUGINJOB_H
#include "acceleratedjob.h"
#include "pluginSupport/OlivePluginInstance.h"
#include "olive/core/util/rational.h"

#include <any>
#include <chrono>

namespace olive
{
namespace plugin
{

class PluginJob : public AcceleratedJob {
public:
	explicit PluginJob(const OFX::Host::ImageEffect::Instance *pluginInstance,
					   const PluginNode *node, NodeValueRow row,
					   const olive::core::rational &time)
		: AcceleratedJob()
		, time_seconds_(time.toDouble())
	{
		this->pluginInstance_ = pluginInstance;
		this->node_ = node;
		Insert(row);
	}
	explicit PluginJob(const OFX::Host::ImageEffect::Instance *pluginInstance,
					   const PluginNode *node, NodeValueRow row)
		: PluginJob(pluginInstance, node, row, olive::core::rational(0))
	{
	}

	PluginNode *node() const
	{
		return const_cast<PluginNode *>(node_);
	}

	OFX::Host::ImageEffect::Instance *pluginInstance()
	{
		return const_cast<OFX::Host::ImageEffect::Instance *>(pluginInstance_);
	}

	double time_seconds() const
	{
		return time_seconds_;
	}

private:
	const OFX::Host::ImageEffect::Instance *pluginInstance_ = nullptr;

	QHash<OfxTime, QHash<QString, std::any>> paramsOnTime;

	QHash<QString, std::any> params;

	const PluginNode *node_ = nullptr;
	double time_seconds_ = 0.0;
};

} // plugin
} // olive

#endif //PLUGINJOB_H
