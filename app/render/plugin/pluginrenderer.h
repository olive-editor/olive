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

//
// Created by mikesolar on 25-10-19.
//

#ifndef PLUGINRENDERER_H
#define PLUGINRENDERER_H

#include <QObject>

#include "render/renderer.h"
#include "render/job/pluginjob.h"

namespace olive
{
namespace plugin
{
namespace detail
{
// 作用：将字节行跨度转换为像素跨度，便于纹理读写。
// Purpose: Convert byte stride to pixel stride for texture I/O.
int BytesToPixels(int byte_linesize, const olive::VideoParams &params);
}
// 作用：OFX 插件渲染器，负责 CPU/GL 路径下的插件调用和纹理桥接。
// Purpose: OFX plugin renderer that drives CPU/GL render paths and texture bridging.
//
// 不再继承 OpenGLRenderer，而是持有一个通用的 Renderer 指针。这样当主渲染器
// 是 Vulkan 或动态加载的后端时，插件仍可通过 CPU readback/upload 路径工作；
// 仅当底层渲染器真正支持 OpenGL 时才走 OFX OpenGL 渲染路径。
class PluginRenderer : public QObject {
	Q_OBJECT
public:
	explicit PluginRenderer(olive::Renderer *renderer,
							QObject *parent = nullptr)
		: QObject(parent)
		, renderer_(renderer)
	{
	}
	virtual ~PluginRenderer() override
	{
	}

	olive::Renderer *renderer() const
	{
		return renderer_;
	}

	// 作用：将目标纹理绑定为插件输出。
	// Purpose: Attach destination texture as OFX output.
	void AttachOutputTexture(olive::TexturePtr texture);
	// 作用：解除目标纹理绑定。
	// Purpose: Detach destination texture binding.
	void DetachOutputTexture();
	// 作用：执行插件渲染流程（参数配置、输入/输出、调用渲染动作）。
	// Purpose: Execute plugin render flow (params, inputs/outputs, render actions).
	void RenderPlugin(TexturePtr src, olive::plugin::PluginJob &job,
					  olive::TexturePtr destination,
					  olive::VideoParams destination_params,
					  bool clear_destination, bool interactive);

private:
	olive::Renderer *renderer_;
};
}
}

#endif //PLUGINRENDERER_H
