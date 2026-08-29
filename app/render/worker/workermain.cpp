/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

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

#include <cstdio>
#include <cstring>
#include <csignal>
#include <memory>
#include <optional>

#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QSurfaceFormat>

#ifdef Q_OS_LINUX
#include <execinfo.h>
#include <unistd.h>
#endif

#include "common/qtutils.h"
#include "config/config.h"
#include "core.h"
#include "node/factory.h"
#include "node/input/multicam/multicamnode.h"
#include "node/project/serializer/serializer.h"
#include "render/diskmanager.h"
#include "render/framemanager.h"
#include "render/ipc/frameslotpool.h"
#include "render/ipc/ipcmessage.h"
#include "render/ipc/sharedmemoryregion.h"
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
#include "render/backend/dynamicrenderer.h"
#endif
#include "render/opengl/openglrenderer.h"
#include "render/rendermanager.h"
#include "render/renderprocessor.h"
#include "render/colorprocessor.h"
#include "render/colortransform.h"

#ifdef Q_OS_MACOS
void HideWorkerDockIcon();
#endif

namespace
{

#ifdef Q_OS_LINUX
void PrintBacktrace(int sig)
{
	void *array[50];
	size_t size = backtrace(array, 50);
	fprintf(stderr, "worker: caught signal %d, backtrace:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	fflush(stderr);
	_exit(128 + sig);
}
#endif

constexpr int kProtocolVersion = 1;
constexpr int kDefaultWidth = 1920;
constexpr int kDefaultHeight = 1080;
constexpr int kDefaultFrameRate = 24;

void InstallSurfaceFormat()
{
	QSurfaceFormat format;
	format.setVersion(3, 2);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setDepthBufferSize(24);
	QSurfaceFormat::setDefaultFormat(format);
}

void LogError(const QString &message)
{
	const QByteArray line = QByteArray("worker: ") + message.toUtf8() + '\n';
	fwrite(line.constData(), 1, size_t(line.size()), stderr);
	fflush(stderr);
}

QJsonObject ErrorMessage(const QString &message, qint64 ticket_id = 0)
{
	QJsonObject o;
	o["type"] = olive::ipc::msgtype::kError;
	o["message"] = message;
	if (ticket_id) {
		o["ticket"] = double(ticket_id);
	}
	return o;
}

class RenderWorker {
public:
	RenderWorker(olive::Renderer *renderer, QFile *out)
		: renderer_(renderer)
		, out_(out)
	{
	}

	~RenderWorker()
	{
		project_.reset();
		olive::ProjectSerializer::Destroy();
		olive::DiskManager::DestroyInstance();
		olive::FrameManager::DestroyInstance();
		olive::NodeFactory::Destroy();
	}

	bool InitializeRuntime()
	{
		// Create a minimal Core instance so that code paths calling Core::instance()
		// (e.g. ViewerOutput::data for timecode display) do not dereference null.
		// The worker is short-lived; leaking this on exit is harmless.
		if (!olive::Core::instance()) {
			new olive::Core(olive::Core::CoreParams());
		}

		olive::Config::Load();
		olive::NodeFactory::Initialize();
		olive::ColorManager::SetUpDefaultConfig();
		olive::FrameManager::CreateInstance();
		olive::DiskManager::CreateInstance();
		olive::ProjectSerializer::Initialize();
		return true;
	}

	bool SendStartupHandshake()
	{
		olive::ipc::HandshakeMsg hs;
		hs.protocol_version = kProtocolVersion;
		hs.shm_key = QString();
		hs.input_shm_key = QString();
		hs.input_slots = 0;
		hs.output_slots = 0;
		hs.slot_data_bytes = 0;
		hs.input_slot_data_bytes = 0;

		QJsonObject handshake = hs.ToJson();
		QOpenGLContext *ctx = nullptr;
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
		if (auto *dynamic_renderer =
				dynamic_cast<olive::DynamicRenderer *>(renderer_)) {
			ctx = dynamic_renderer->OpenGLContext();
		} else
#endif
		{
			ctx = static_cast<olive::OpenGLRenderer *>(renderer_)->context();
		}
		if (ctx) {
			const QSurfaceFormat fmt = ctx->format();
			handshake["gl_major"] = fmt.majorVersion();
			handshake["gl_minor"] = fmt.minorVersion();
		}

		return Write(handshake);
	}

	bool Handle(const QJsonObject &message)
	{
		const QString type = message["type"].toString();

		if (type == QLatin1String(olive::ipc::msgtype::kHandshake)) {
			olive::ipc::HandshakeMsg hs;
			if (!olive::ipc::HandshakeMsg::FromJson(message, &hs)) {
				return Write(
					ErrorMessage(QStringLiteral("invalid handshake message")));
			}
			return AttachOutputPool(hs);
		}

		if (type == QLatin1String(olive::ipc::msgtype::kLoadGraph)) {
			olive::ipc::LoadGraphMsg load;
			if (!olive::ipc::LoadGraphMsg::FromJson(message, &load)) {
				return Write(
					ErrorMessage(QStringLiteral("invalid load_graph message")));
			}
			return LoadGraph(load.path);
		}

		if (type == QLatin1String(olive::ipc::msgtype::kRenderFrame)) {
			olive::ipc::RenderFrameMsg render;
			if (!olive::ipc::RenderFrameMsg::FromJson(message, &render)) {
				return Write(ErrorMessage(
					QStringLiteral("invalid render_frame message")));
			}
			return RenderFrame(render);
		}

		if (type == QLatin1String(olive::ipc::msgtype::kCancel)) {
			// Stage 5 wires cancellation into in-flight jobs. Stage 2 has only synchronous single-frame work.
			return true;
		}

		if (type == QLatin1String(olive::ipc::msgtype::kShutdown)) {
			shutdown_requested_ = true;
			return true;
		}

		return Write(
			ErrorMessage(QStringLiteral("unknown message type: %1").arg(type)));
	}

	bool shutdown_requested() const
	{
		return shutdown_requested_;
	}

private:
	bool Write(const QJsonObject &message)
	{
		const bool ok = olive::ipc::WriteMessage(out_, message);
		out_->flush();
		return ok;
	}

	bool AttachOutputPool(const olive::ipc::HandshakeMsg &hs)
	{
		if (hs.protocol_version != kProtocolVersion) {
			return Write(
				ErrorMessage(QStringLiteral("unsupported protocol version %1")
								 .arg(hs.protocol_version)));
		}

		if (hs.shm_key.isEmpty() || hs.output_slots <= 0 ||
			hs.slot_data_bytes <= 0) {
			return Write(ErrorMessage(QStringLiteral(
				"handshake missing output shared-memory geometry")));
		}

		const size_t bytes = olive::ipc::FrameSlotPool::BytesNeeded(
			uint32_t(hs.output_slots), size_t(hs.slot_data_bytes));
		if (!output_region_.Open(hs.shm_key, bytes,
								 olive::ipc::SharedMemoryRegion::kAttach)) {
			return Write(ErrorMessage(
				QStringLiteral("failed to attach shared memory: %1")
					.arg(output_region_.error())));
		}

		output_pool_ = olive::ipc::FrameSlotPool::Attach(output_region_.data());
		if (!output_pool_->IsValid()) {
			output_region_.Close();
			output_pool_.reset();
			return Write(ErrorMessage(QStringLiteral(
				"shared memory does not contain a frame slot pool")));
		}

		input_pool_.reset();
		input_region_.Close();
		if (hs.input_slots > 0) {
			if (hs.input_shm_key.isEmpty() || hs.input_slot_data_bytes <= 0) {
				return Write(ErrorMessage(QStringLiteral(
					"handshake missing input shared-memory geometry")));
			}

			const size_t input_bytes = olive::ipc::FrameSlotPool::BytesNeeded(
				uint32_t(hs.input_slots), size_t(hs.input_slot_data_bytes));
			if (!input_region_.Open(hs.input_shm_key, input_bytes,
									olive::ipc::SharedMemoryRegion::kAttach)) {
				return Write(ErrorMessage(
					QStringLiteral("failed to attach input shared memory: %1")
						.arg(input_region_.error())));
			}

			input_pool_ =
				olive::ipc::FrameSlotPool::Attach(input_region_.data());
			if (!input_pool_->IsValid()) {
				input_region_.Close();
				input_pool_.reset();
				return Write(ErrorMessage(QStringLiteral(
					"input shared memory does not contain a frame slot pool")));
			}
		}

		return true;
	}

	bool LoadGraph(const QString &path)
	{
		{
			QFileInfo fi(path);
			if (!fi.exists()) {
				LogError(
					QStringLiteral("LoadGraph: graph file does not exist: %1")
						.arg(path));
				return Write(ErrorMessage(
					QStringLiteral("graph file does not exist: %1").arg(path)));
			}
			if (fi.size() == 0) {
				LogError(QStringLiteral("LoadGraph: graph file is empty: %1")
							 .arg(path));
				return Write(ErrorMessage(
					QStringLiteral("graph file is empty: %1").arg(path)));
			}
			LogError(
				QStringLiteral("LoadGraph: loading %1 (%2 bytes, readable=%3)")
					.arg(path)
					.arg(fi.size())
					.arg(fi.isReadable()));
		}

		auto loaded = std::make_unique<olive::Project>();
		// Do not call Initialize() here: project serializers expect a blank
		// project (root_ == nullptr) and will set root themselves. Calling
		// Initialize() first triggers Q_ASSERT(!root_) in Project::Load.

		olive::ProjectSerializer::Result result =
			olive::ProjectSerializer::Load(loaded.get(), path,
										   olive::ProjectSerializer::kProject);
		if (result != olive::ProjectSerializer::kSuccess) {
			return Write(
				ErrorMessage(QStringLiteral("failed to load graph %1: %2")
								 .arg(path, result.GetDetails())));
		}

		project_ = std::move(loaded);
		node_by_token_.clear();
		color_processor_cache_.clear();

		const auto &data = result.GetLoadData();
		for (auto it = data.node_ptrs.cbegin(); it != data.node_ptrs.cend();
			 ++it) {
			node_by_token_.insert(QString::number(it.key()), it.value());
		}
		for (auto it = data.node_uuids.cbegin(); it != data.node_uuids.cend();
			 ++it) {
			node_by_token_.insert(it.value().toString(), it.key());
			node_by_token_.insert(it.value().toString(QUuid::WithoutBraces),
								  it.key());
		}

		QJsonObject ack;
		ack["type"] = QStringLiteral("graph_loaded");
		ack["nodes"] = node_by_token_.size();
		return Write(ack);
	}

	olive::Node *FindNode(const QString &token) const
	{
		if (olive::Node *node = node_by_token_.value(token, nullptr)) {
			return node;
		}

		bool ok = false;
		const quintptr ptr = token.toULongLong(&ok, 0);
		if (ok) {
			return node_by_token_.value(QString::number(ptr), nullptr);
		}

		return nullptr;
	}

	bool RenderFrame(const olive::ipc::RenderFrameMsg &message)
	{
		if (!project_) {
			return Write(ErrorMessage(
				QStringLiteral("render_frame received before load_graph"),
				message.ticket_id));
		}
		if (!output_pool_ || !output_pool_->IsValid()) {
			return Write(ErrorMessage(
				QStringLiteral(
					"render_frame received before output shm handshake"),
				message.ticket_id));
		}

		olive::Node *node = FindNode(message.node_uuid);
		if (!node) {
			return Write(
				ErrorMessage(QStringLiteral("render node not found: %1")
								 .arg(message.node_uuid),
							 message.ticket_id));
		}

		QVector<int> input_slots;
		const QVector<int> requested_input_slots =
			message.input_slots.isEmpty() && message.input_slot >= 0 ?
				QVector<int>{ message.input_slot } :
				message.input_slots;
		if (!requested_input_slots.isEmpty()) {
			if (!input_pool_ || !input_pool_->IsValid()) {
				return Write(ErrorMessage(
					QStringLiteral(
						"render_frame referenced input slot without input pool"),
					message.ticket_id));
			}

			for (int requested_slot : requested_input_slots) {
				if (requested_slot < 0 ||
					requested_slot >= int(input_pool_->slot_count())) {
					for (int slot : input_slots) {
						input_pool_->Release(uint32_t(slot));
					}
					return Write(ErrorMessage(
						QStringLiteral("input slot index out of range"),
						message.ticket_id));
				}

				uint32_t consumed_slot = 0;
				if (!input_pool_->Consume(&consumed_slot)) {
					for (int slot : input_slots) {
						input_pool_->Release(uint32_t(slot));
					}
					return Write(
						ErrorMessage(QStringLiteral("input slot was not ready"),
									 message.ticket_id));
				}
				if (int(consumed_slot) != requested_slot) {
					input_pool_->Release(consumed_slot);
					for (int slot : input_slots) {
						input_pool_->Release(uint32_t(slot));
					}
					return Write(ErrorMessage(
						QStringLiteral("input slot order mismatch"),
						message.ticket_id));
				}
				input_slots.append(int(consumed_slot));

				const olive::ipc::FrameSlotMeta *meta =
					input_pool_->Meta(consumed_slot);
				if (meta) {
				}
			}
		}

		olive::VideoParams vparams(
			message.width > 0 ? message.width : kDefaultWidth,
			message.height > 0 ? message.height : kDefaultHeight,
			olive::rational(1, kDefaultFrameRate),
			message.format >= 0 ? olive::PixelFormat::Format(message.format) :
								  olive::PixelFormat::F32,
			message.channel_count > 0 ? message.channel_count :
										olive::VideoParams::kRGBAChannelCount);

		olive::RenderTicketPtr ticket = std::make_shared<olive::RenderTicket>();
		ticket->setProperty("node", olive::QtUtils::PtrToValue(node));
		ticket->setProperty("time",
							QVariant::fromValue(olive::rational(
								int(message.time_num), int(message.time_den))));
		ticket->setProperty("size", QSize(message.width, message.height));
		ticket->setProperty("matrix", QMatrix4x4());
		ticket->setProperty("format",
							message.format >= 0 ?
								olive::PixelFormat::Format(message.format) :
								olive::PixelFormat::INVALID);
		ticket->setProperty("usecache", false);
		ticket->setProperty("channelcount", message.channel_count);
		ticket->setProperty("mode", olive::RenderMode::Mode(message.mode));
		ticket->setProperty("type", olive::RenderManager::kTypeVideo);
		ticket->setProperty("colormanager", olive::QtUtils::PtrToValue(
												project_->color_manager()));

		{
			olive::ColorProcessorPtr color_output;
			if (message.has_color_transform) {
				QString cache_key = QStringLiteral("%1|%2|%3|%4")
										.arg(message.color_is_display ? 1 : 0)
										.arg(message.color_output,
											 message.color_view,
											 message.color_look);
				auto it = color_processor_cache_.find(cache_key);
				if (it != color_processor_cache_.end()) {
					color_output = it.value();
				} else {
					olive::ColorTransform transform;
					if (message.color_is_display) {
						transform = olive::ColorTransform(message.color_output,
														  message.color_view,
														  message.color_look);
					} else {
						transform = olive::ColorTransform(message.color_output);
					}
					color_output = olive::ColorProcessor::Create(
						project_->color_manager(),
						project_->color_manager()->GetReferenceColorSpace(),
						transform);
					if (color_output) {
						color_processor_cache_.insert(cache_key, color_output);
					}
				}
			}
			ticket->setProperty("coloroutput",
								QVariant::fromValue(color_output));
		}
		ticket->setProperty("vparam", QVariant::fromValue(vparams));
		ticket->setProperty("aparam",
							QVariant::fromValue(olive::AudioParams()));
		ticket->setProperty("return", olive::RenderManager::kFrame);
		ticket->setProperty("cache", QString());
		ticket->setProperty("cachetimebase",
							QVariant::fromValue(olive::rational(1)));
		ticket->setProperty("cacheid", QVariant::fromValue(QUuid()));
		ticket->setProperty("multicam", olive::QtUtils::PtrToValue(
											static_cast<void *>(nullptr)));
		ticket->setProperty(
			"ipc_input_pool",
			olive::QtUtils::PtrToValue(input_pool_ ?
										   static_cast<void *>(&*input_pool_) :
										   static_cast<void *>(nullptr)));
		QVariantList input_slot_values;
		for (int slot : input_slots) {
			input_slot_values.append(slot);
		}
		ticket->setProperty("ipc_input_slots", input_slot_values);
		ticket->setProperty("ipc_input_slot_cursor", 0);
		ticket->setProperty("ipc_input_slot",
							input_slots.isEmpty() ? -1 : input_slots.front());

		ticket->Start();
		olive::RenderProcessor::Process(ticket, renderer_, nullptr,
										&shader_cache_);
		for (int slot : input_slots) {
			input_pool_->Release(uint32_t(slot));
		}
		if (!ticket->HasResult()) {
			return Write(ErrorMessage(
				QStringLiteral("render produced no frame"), message.ticket_id));
		}

		olive::FramePtr frame = ticket->Get().value<olive::FramePtr>();
		if (!frame || !frame->is_allocated()) {
			return Write(ErrorMessage(QStringLiteral("render result was empty"),
									  message.ticket_id));
		}

		uint32_t slot = 0;
		if (!output_pool_->Acquire(&slot)) {
			return Write(
				ErrorMessage(QStringLiteral("no free output frame slot"),
							 message.ticket_id));
		}

		const int data_size = frame->linesize_bytes() * frame->height();
		if (data_size > int(output_pool_->slot_data_bytes())) {
			output_pool_->Release(slot);
			LogError(QString("Output frame size") + QString::number(data_size));
			LogError(QString("Slot size") +
					 QString::number(output_pool_->slot_data_bytes()));
			return Write(ErrorMessage(
				QStringLiteral("rendered frame does not fit output slot "),
				message.ticket_id));
		}

		std::memcpy(output_pool_->SlotData(slot), frame->const_data(),
					size_t(data_size));
		olive::ipc::FrameSlotMeta *meta = output_pool_->Meta(slot);
		meta->id = message.ticket_id;
		meta->time_num = frame->timestamp().numerator();
		meta->time_den = frame->timestamp().denominator();
		meta->width = frame->width();
		meta->height = frame->height();
		meta->format = int32_t(frame->format());
		meta->channel_count = frame->channel_count();
		meta->linesize = frame->linesize_bytes();
		meta->data_size = data_size;

		if (!output_pool_->Publish(slot)) {
			output_pool_->Release(slot);
			return Write(ErrorMessage(
				QStringLiteral("failed to publish output frame slot"),
				message.ticket_id));
		}
		olive::ipc::FrameReadyMsg ready;
		ready.ticket_id = message.ticket_id;
		ready.output_slot = int(slot);
		return Write(ready.ToJson());
	}

	olive::Renderer *renderer_;
	QFile *out_;
	bool shutdown_requested_ = false;
	std::unique_ptr<olive::Project> project_;
	QHash<QString, olive::Node *> node_by_token_;
	olive::ipc::SharedMemoryRegion output_region_;
	std::optional<olive::ipc::FrameSlotPool> output_pool_;
	olive::ipc::SharedMemoryRegion input_region_;
	std::optional<olive::ipc::FrameSlotPool> input_pool_;
	olive::ShaderCache shader_cache_;
	QHash<QString, olive::ColorProcessorPtr> color_processor_cache_;
};

} // namespace

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	InstallSurfaceFormat();

	QGuiApplication app(argc, argv);

#ifdef Q_OS_MACOS
	HideWorkerDockIcon();
#endif

	QCoreApplication::setOrganizationName(QStringLiteral("oakvideoeditor.org"));
	QCoreApplication::setApplicationName(QStringLiteral("oak-render-worker"));

	QString backend = QStringLiteral("opengl");
	const QStringList args = app.arguments();
	for (int i = 1; i < args.size(); ++i) {
		if (args[i] == QStringLiteral("--backend") && i + 1 < args.size()) {
			backend = args[i + 1].toLower();
			++i;
		}
	}

#ifdef Q_OS_LINUX
	std::signal(SIGSEGV, PrintBacktrace);
	std::signal(SIGABRT, PrintBacktrace);
	std::signal(SIGFPE, PrintBacktrace);
#endif

	QFile in;
	QFile out;
	if (!in.open(stdin, QIODevice::ReadOnly | QIODevice::Unbuffered) ||
		!out.open(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered)) {
		LogError(QStringLiteral("failed to open stdio control pipes"));
		return 1;
	}

	olive::Renderer *renderer;
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	auto *dynamic_renderer = new olive::DynamicRenderer(backend);
	if (dynamic_renderer->Init()) {
		dynamic_renderer->PostInit();
		renderer = dynamic_renderer;
	} else {
		delete dynamic_renderer;
		qWarning() << "Failed to initialize dynamic" << backend
				   << "backend, falling back to direct OpenGL renderer";
		renderer = new olive::OpenGLRenderer();
		if (!renderer->Init()) {
			LogError(QStringLiteral("failed to initialize OpenGL renderer"));
			delete renderer;
			return 1;
		}
		renderer->PostInit();
	}
#else
	renderer = new olive::OpenGLRenderer();
	if (!renderer->Init()) {
		LogError(QStringLiteral("failed to initialize OpenGL renderer"));
		delete renderer;
		return 1;
	}
	renderer->PostInit();
#endif

	// Validate the renderer. For OpenGL we check the GL context; for Vulkan we
	// rely on Init()/PostInit() succeeding (there is no QOpenGLContext).
	bool renderer_valid = true;
	QOpenGLContext *ctx = nullptr;
	if (backend == QStringLiteral("opengl")) {
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
		if (auto *loaded_renderer =
				dynamic_cast<olive::DynamicRenderer *>(renderer)) {
			ctx = loaded_renderer->OpenGLContext();
		} else
#endif
		{
			ctx = static_cast<olive::OpenGLRenderer *>(renderer)->context();
		}
		if (!ctx || !ctx->isValid()) {
			renderer_valid = false;
		}
	}
	if (!renderer_valid) {
		LogError(QStringLiteral("OpenGL context is not valid after init"));
		renderer->Destroy();
		renderer->PostDestroy();
		delete renderer;
		return 1;
	}

	int exit_code = 0;
	{
		RenderWorker worker(renderer, &out);
		if (!worker.InitializeRuntime() || !worker.SendStartupHandshake()) {
			exit_code = 1;
		} else {
			QByteArray buffer;
			while (!worker.shutdown_requested() && !in.atEnd()) {
				const QByteArray chunk = in.readLine();
				if (chunk.isEmpty()) {
					break;
				}

				buffer.append(chunk);
				while (true) {
					QJsonObject message;
					bool ok = true;
					if (!olive::ipc::ReadMessage(&buffer, &message, &ok)) {
						if (!ok) {
							olive::ipc::WriteMessage(
								&out, ErrorMessage(QStringLiteral(
										  "malformed control message")));
							out.flush();
							continue;
						}
						break;
					}

					if (!worker.Handle(message)) {
						exit_code = 1;
						break;
					}
				}
			}
		}
	}

	renderer->Destroy();
	renderer->PostDestroy();
	delete renderer;

	return exit_code;
}
