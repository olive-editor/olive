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

#include "rendermanager.h"

#include <QApplication>
#include <QMatrix4x4>
#include <QThread>

#include "config/config.h"
#include "core.h"
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
#include "render/backend/dynamicrenderer.h"
#endif
#include "render/opengl/openglrenderer.h"
#include "renderprocessor.h"
#include "renderworkerpool.h"
#include "task/conform/conform.h"
#include "task/taskmanager.h"
#include "window/mainwindow/mainwindow.h"

namespace olive
{

RenderManager *RenderManager::instance_ = nullptr;
const rational RenderManager::kDryRunInterval = rational(10);

RenderManager::Backend RenderManager::BackendFromString(const QString &backend)
{
	const QString lower = backend.toLower();
	if (lower == QStringLiteral("vulkan")) {
		return kVulkan;
	}

	if (lower == QStringLiteral("multiprocess")) {
		return kMultiProcess;
	}

	if (lower == QStringLiteral("dummy")) {
		return kDummy;
	}

	return kOpenGL;
}

QString RenderManager::BackendToString(Backend backend)
{
	switch (backend) {
	case kOpenGL:
		return QStringLiteral("opengl");
	case kVulkan:
		return QStringLiteral("vulkan");
	case kMultiProcess:
		return QStringLiteral("multiprocess");
	case kDummy:
		return QStringLiteral("dummy");
	}

	return QStringLiteral("opengl");
}

RenderManager::RenderManager(QObject *parent)
	: backend_(BackendFromString(OLIVE_CONFIG("GraphicsBackend").toString()))
	, requested_backend_(backend_)
	, aggressive_gc_(0)
	, worker_pool_(nullptr)
{
	if (backend_ == kVulkan) {
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
		qWarning()
			<< "Vulkan backend requested but dynamic render backend is not enabled. Falling back to OpenGL.";
		backend_ = kOpenGL;
#endif
	}

	if (backend_ == kOpenGL || backend_ == kVulkan) {
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
		auto *dynamic_renderer =
			new DynamicRenderer(BackendToString(requested_backend_));
		if (!dynamic_renderer->Load()) {
			qWarning() << "Failed to load dynamic render backend"
					   << BackendToString(requested_backend_)
					   << ", falling back to OpenGL";
			delete dynamic_renderer;
			backend_ = kOpenGL;
			context_ = new OpenGLRenderer();
		} else {
			context_ = dynamic_renderer;
			// DynamicRenderer may internally fall back (e.g. Vulkan -> OpenGL).
			// Synchronize RenderManager's view of the actual runtime backend.
			Backend actual_backend =
				BackendFromString(dynamic_renderer->backend_name());
			if (actual_backend != backend_) {
				qWarning() << "Dynamic render backend fell back from"
						   << BackendToString(backend_) << "to"
						   << BackendToString(actual_backend);
				backend_ = actual_backend;
			}
		}
#else
		context_ = new OpenGLRenderer();
#endif
		decoder_cache_ = new DecoderCache();
		shader_cache_ = new ShaderCache();
	} else {
		qCritical() << "Tried to initialize unknown graphics backend";
		context_ = nullptr;
		decoder_cache_ = nullptr;
	}

	if (context_) {
		dry_run_thread_ = CreateThread();
		audio_thread_ = CreateThread();

		waveform_threads_.resize(QThread::idealThreadCount());
		for (size_t i = 0; i < waveform_threads_.size(); i++) {
			waveform_threads_[i] = CreateThread();
		}

		auto_cacher_ = new PreviewAutoCacher(this);

		worker_pool_ = new RenderWorkerPool(
			decoder_cache_, BackendToString(requested_backend_), this);
		worker_pool_->start(QThread::NormalPriority);
		backend_ = kMultiProcess;
	}

	decoder_clear_timer_ = new QTimer(this);
	decoder_clear_timer_->setInterval(kDecoderMaximumInactivity);
	connect(decoder_clear_timer_, &QTimer::timeout, this,
			&RenderManager::ClearOldDecoders);
	decoder_clear_timer_->start();
}

RenderManager::~RenderManager()
{
	if (context_) {
		if (worker_pool_) {
			worker_pool_->Shutdown();
			delete worker_pool_;
			worker_pool_ = nullptr;
		}

		delete shader_cache_;
		delete decoder_cache_;

		for (RenderThread *rt : render_threads_) {
			rt->quit();
			rt->wait();
		}

		context_->PostDestroy();
		delete context_;
	}
}

RenderThread *RenderManager::CreateThread(Renderer *renderer)
{
	auto t = new RenderThread(renderer, decoder_cache_, shader_cache_, this);
	render_threads_.push_back(t);
	t->start(QThread::NormalPriority);
	return t;
}

RenderTicketPtr RenderManager::RenderFrame(const RenderVideoParams &params)
{
	// Create ticket
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();

	ticket->setProperty("node", QtUtils::PtrToValue(params.node));
	ticket->setProperty("time", QVariant::fromValue(params.time));
	ticket->setProperty("size", params.force_size);
	ticket->setProperty("matrix", params.force_matrix);
	ticket->setProperty("format",
						static_cast<PixelFormat::Format>(params.force_format));
	ticket->setProperty("usecache", params.use_cache);
	ticket->setProperty("channelcount", params.force_channel_count);
	ticket->setProperty("mode", params.mode);
	ticket->setProperty("type", kTypeVideo);
	ticket->setProperty("colormanager",
						QtUtils::PtrToValue(params.color_manager));
	ticket->setProperty("coloroutput",
						QVariant::fromValue(params.force_color_output));
	ticket->setProperty("colortransform",
						QVariant::fromValue(params.force_color_transform));
	Q_ASSERT(params.video_params.is_valid());
	ticket->setProperty("vparam", QVariant::fromValue(params.video_params));
	ticket->setProperty("aparam", QVariant::fromValue(params.audio_params));
	ticket->setProperty("return", params.return_type);
	ticket->setProperty("cache", params.cache_dir);
	ticket->setProperty("cachetimebase",
						QVariant::fromValue(params.cache_timebase));
	ticket->setProperty("cacheid", QVariant::fromValue(params.cache_id));
	ticket->setProperty("multicam", QtUtils::PtrToValue(params.multicam));

	// Video frames are always rendered by the worker pool. GPU textures cannot
	// be shared across the process boundary (or across independent Vulkan
	// instances), so texture-return requests are downgraded to CPU frames.
	RenderVideoParams worker_params = params;
	if (worker_params.return_type == ReturnType::kTexture) {
		worker_params.return_type = ReturnType::kFrame;
	}

	if (worker_params.return_type == ReturnType::kNull) {
		if (dry_run_thread_) {
			dry_run_thread_->AddTicket(ticket);
		} else {
			// No render threads (e.g. dummy backend), finish without a result
			ticket->Finish();
		}
	} else if (worker_pool_ &&
			   worker_pool_->SubmitFrame(ticket, worker_params)) {
		return ticket;
	} else {
		qWarning()
			<< "RenderManager: worker pool unavailable, finishing ticket "
			   "without result";
		ticket->Finish();
	}

	return ticket;
}

RenderTicketPtr RenderManager::RenderAudio(const RenderAudioParams &params)
{
	// Create ticket
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();

	ticket->setProperty("node", QtUtils::PtrToValue(params.node));
	ticket->setProperty("time", QVariant::fromValue(params.range));
	ticket->setProperty("type", kTypeAudio);
	ticket->setProperty("enablewaveforms", params.generate_waveforms);
	ticket->setProperty("clamp", params.clamp);
	ticket->setProperty("aparam", QVariant::fromValue(params.audio_params));
	ticket->setProperty("mode", params.mode);

	if (params.generate_waveforms && !waveform_threads_.empty()) {
		size_t thread_index = last_waveform_thread_ % waveform_threads_.size();
		RenderThread *thread = waveform_threads_[thread_index];
		thread->AddTicket(ticket);
		last_waveform_thread_++;
	} else if (audio_thread_) {
		audio_thread_->AddTicket(ticket);
	} else {
		// No render threads (e.g. dummy backend), finish without a result
		ticket->Finish();
	}

	return ticket;
}

bool RenderManager::RemoveTicket(RenderTicketPtr ticket)
{
	if (worker_pool_ && worker_pool_->RemoveTicket(ticket)) {
		return true;
	}

	for (RenderThread *rt : render_threads_) {
		if (rt->RemoveTicket(ticket)) {
			return true;
		}
	}

	return false;
}

void RenderManager::SetAggressiveGarbageCollection(bool enabled)
{
	aggressive_gc_ += enabled ? 1 : -1;

	// Clamp at zero so unbalanced disable calls can't drive the counter negative
	if (aggressive_gc_ < 0) {
		aggressive_gc_ = 0;
	}

	if (aggressive_gc_ > 0) {
		decoder_clear_timer_->setInterval(kDecoderMaximumInactivityAggressive);
	} else {
		decoder_clear_timer_->setInterval(kDecoderMaximumInactivity);
	}
}

void RenderManager::ClearOldDecoders()
{
	if (!decoder_cache_) {
		// No decoder cache exists on backends without a renderer (e.g. dummy)
		return;
	}

	QMutexLocker locker(decoder_cache_->mutex());

	qint64 min_age =
		QDateTime::currentMSecsSinceEpoch() - kDecoderMaximumInactivity;

	for (auto it = decoder_cache_->begin(); it != decoder_cache_->end();) {
		DecoderPair decoder = it.value();

		if (decoder.decoder->GetLastAccessedTime() < min_age) {
			decoder.decoder->Close();
			it = decoder_cache_->erase(it);
		} else {
			it++;
		}
	}
}

RenderThread::RenderThread(Renderer *renderer, DecoderCache *decoder_cache,
						   ShaderCache *shader_cache, QObject *parent)
	: QThread(parent)
	, cancelled_(false)
	, context_(renderer)
	, decoder_cache_(decoder_cache)
	, shader_cache_(shader_cache)
{
	if (context_) {
		context_->Init();
		context_->moveToThread(this);
	}
}

void RenderThread::AddTicket(RenderTicketPtr ticket)
{
	QMutexLocker locker(&mutex_);
	ticket->moveToThread(this);
	queue_.push_back(ticket);
	wait_.wakeOne();
}

bool RenderThread::RemoveTicket(RenderTicketPtr ticket)
{
	QMutexLocker locker(&mutex_);

	auto it = std::find(queue_.begin(), queue_.end(), ticket);
	if (it == queue_.end()) {
		return false;
	}

	queue_.erase(it);
	return true;
}

void RenderThread::quit()
{
	QMutexLocker locker(&mutex_);
	cancelled_ = true;
	wait_.wakeOne();
}

void RenderThread::run()
{
	if (context_) {
		context_->PostInit();
	}

	QMutexLocker locker(&mutex_);

	while (!cancelled_) {
		if (queue_.empty()) {
			wait_.wait(&mutex_);
		}

		if (cancelled_) {
			break;
		}

		if (!queue_.empty()) {
			RenderTicketPtr ticket = queue_.front();
			queue_.pop_front();

			locker.unlock();

			// Setup the ticket for ::Process
			ticket->Start();

			if (ticket->IsCancelled()) {
				ticket->Finish();
			} else {
				RenderProcessor::Process(ticket, context_, decoder_cache_,
										 shader_cache_);
			}

			locker.relock();
		}
	}

	if (context_) {
		context_->Destroy();
		context_->moveToThread(this->thread());
	}
}

}
