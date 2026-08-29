/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include "renderer.h"

#include <QDateTime>
#include <QThread>
#include <QTimer>
#include <QVector2D>

namespace olive
{

Renderer::Renderer(QObject *parent)
	: QObject(parent)
	, lifetime_(std::make_shared<RendererLifetime>())
{
}

Renderer::~Renderer()
{
	destroyed_ = true;
	if (lifetime_) {
		lifetime_->alive = false;
	}
}

TexturePtr Renderer::CreateTexture(const VideoParams &params, const void *data,
								   int linesize)
{
	QVariant v;

	if (USE_TEXTURE_CACHE) {
		QMutexLocker locker(&texture_cache_lock_);
		for (auto it = texture_cache_.begin(); it != texture_cache_.end();
			 it++) {
			if (it->width == params.effective_width() &&
				it->height == params.effective_height() &&
				it->depth == params.effective_depth() &&
				it->format == params.format() &&
				it->channel_count == params.channel_count()) {
				v = it->handle;
				texture_cache_.erase(it);
				break;
			}
		}
	}

	if (v.isNull()) {
		v = CreateNativeTexture(params.effective_width(),
								params.effective_height(),
								params.effective_depth(), params.format(),
								params.channel_count(), data, linesize);
	} else if (data) {
		UploadToTexture(v, params, data, linesize);
	} else {
		this->Flush();
	}

	return CreateTextureFromNativeHandle(v, params);
}

void Renderer::DestroyTexture(Texture *texture)
{
	if (destroyed_) {
		return;
	}
	if (USE_TEXTURE_CACHE) {
		// HACK: Dirty, dirty hack. OpenGL uses "contexts" to store all of its data, and each context
		//       can only be used by the thread that created it. However there are also "shared contexts"
		//       where assets from one context can be used in another. We use shared contexts so that
		//       textures rendered in the background can be displayed on the screen, travelling from
		//       a background thread to the main UI thread. However, when that texture is destroyed, it
		//       comes back here to be placed in the texture cache. But that leads to a race condition
		//       because it will call the background thread's renderer in the main thread. Since all
		//       assets are shared, we could technically just get the texture to call "destroy" in the
		//       viewer's renderer instance, but that would mean all textures would end up stranded
		//       there unusable by the background renderer, negating the very advantage of the texture
		//       cache in the first place. Therefore, we simply allow the thread calling to happen, and
		//       use mutexes to prevent race conditions.
		//
		//       Presumably Vulkan would not have this issue because it allows for application-wide
		//       instances and multithreading.
		texture_cache_lock_.lock();
		texture_cache_.push_back(
			{ texture->params().effective_width(),
			  texture->params().effective_height(),
			  texture->params().effective_depth(), texture->params().format(),
			  texture->params().channel_count(), texture->id(),
			  QDateTime::currentMSecsSinceEpoch() });
		texture_cache_lock_.unlock();

		if (QThread::currentThread() == this->thread()) {
			ClearOldTextures();
		}
	} else {
		DestroyNativeTexture(texture->id());
	}
}

QVariant Renderer::GetDefaultShader()
{
	QMutexLocker locker(&color_cache_mutex_);

	if (default_shader_.isNull()) {
		default_shader_ = CreateNativeShader(ShaderCode(QString(), QString()));
	}

	return default_shader_;
}

void Renderer::Destroy()
{
	if (!default_shader_.isNull()) {
		DestroyNativeShader(default_shader_);
		default_shader_.clear();
	}

	{
		QMutexLocker locker(&color_cache_mutex_);

		// Destroy the cached native shaders explicitly. The LUT textures are
		// TexturePtrs whose destructors call DestroyTexture(), so the cache must
		// be cleared while the renderer is still alive for those to be honored.
		for (auto it = color_cache_.begin(); it != color_cache_.end(); it++) {
			if (!it->compiled_shader.isNull()) {
				DestroyNativeShader(it->compiled_shader);
			}
		}
		color_cache_.clear();
	}

	if (!interlace_texture_.isNull()) {
		DestroyNativeShader(interlace_texture_);
		interlace_texture_.clear();
	}

	for (auto it = texture_cache_.begin(); it != texture_cache_.end(); it++) {
		DestroyNativeTexture(it->handle);
	}
	texture_cache_.clear();

	destroyed_ = true;
	if (lifetime_) {
		lifetime_->alive = false;
	}

	DestroyInternal();
}

TexturePtr Renderer::CreateTextureFromNativeHandle(const QVariant &v,
												   const VideoParams &params)
{
	if (v.isNull()) {
		return nullptr;
	}

	return std::make_shared<Texture>(this, v, params, lifetime_);
}

void Renderer::ClearOldTextures()
{
	QMutexLocker locker(&texture_cache_lock_);

	for (auto it = texture_cache_.begin(); it != texture_cache_.end();) {
		if (it->accessed <
			QDateTime::currentMSecsSinceEpoch() - MAX_TEXTURE_LIFE) {
			DestroyNativeTexture(it->handle);
			it = texture_cache_.erase(it);
		} else {
			it++;
		}
	}
}

} // namespace olive
