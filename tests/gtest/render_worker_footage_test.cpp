/*
 * Oak Video Editor - Render Worker Footage Integration Test
 * Copyright (C) 2026 Oak Team
 *
 * End-to-end test that spawns oak-render-worker, feeds it a real decoded
 * frame from tests/demo.mp4 through the IPC shared-memory frame pool, and
 * verifies that the worker returns a non-black output frame.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QThread>

#include <memory>

#include "codec/decoder.h"
#include "codec/frame.h"
#include "common/filefunctions.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "node/project/serializer/serializer.h"
#include "render/diskmanager.h"
#include "render/ipc/frameslotpool.h"
#include "render/ipc/ipcmessage.h"
#include "render/ipc/sharedmemoryregion.h"
#include "render/videoparams.h"

#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
#include "render/backend/dynamicrenderer.h"
#include "render/backend/renderbackend_c.h"
#endif

using namespace olive;
using namespace olive::core;

namespace
{

#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
bool IsRenderBackendAvailable(const QString &backend)
{
	olive::DynamicRenderer renderer(backend);
	if (!renderer.Load()) {
		return false;
	}

	OakRenderBackendInfo info = {};
	if (!renderer.GetBackendInfo(&info)) {
		return false;
	}

	if (backend == QStringLiteral("vulkan") &&
		info.kind != OAK_RENDER_BACKEND_VULKAN) {
		return false;
	}

	if (backend == QStringLiteral("opengl") &&
		info.kind != OAK_RENDER_BACKEND_OPENGL) {
		return false;
	}

	return renderer.Init();
}
#else
bool IsRenderBackendAvailable(const QString &)
{
	// When the dynamic backend is not built, the test binary is linked
	// directly against the renderer and we have no way to probe it cheaply
	// from here. These tests require a working GPU backend, so skip them
	// unless we can verify availability through the dynamic adapter.
	return false;
}
#endif

constexpr int kInputSlots = 1;
constexpr int kOutputSlots = 1;
constexpr int kTimeoutMs = 30000;

QString WorkerBinaryPath()
{
	// The test binary lives in cmake-build-debug/tests/gtest; the worker is in
	// cmake-build-debug/app.
	QDir dir(QCoreApplication::applicationDirPath());
	dir.cdUp(); // tests/gtest -> tests
	dir.cdUp(); // tests -> build dir
	dir.cd(QStringLiteral("app"));
#if defined(_WIN32)
	return dir.filePath(QStringLiteral("oak-render-worker.exe"));
#else
	return dir.filePath(QStringLiteral("oak-render-worker"));
#endif
}

QString DemoVideoPath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/demo.mp4"));
}

double SampleBrightnessF32(const void *data, int width, int height, int stride)
{
	const auto *base = reinterpret_cast<const uint8_t *>(data);
	double avg = 0.0;
	int samples = 0;
	for (int y = 0; y < height && y < 1080; y += 120) {
		for (int x = 0; x < width && x < 1920; x += 240) {
			const auto *p = reinterpret_cast<const float *>(
				base + y * stride + x * 4 * sizeof(float));
			for (int c = 0; c < 3; ++c) {
				avg += p[c];
			}
			samples += 3;
		}
	}
	return samples > 0 ? avg / samples : 0.0;
}

void SaveFrameAsPng(const void *data, int width, int height,
					const QString &path)
{
	QImage img(width, height, QImage::Format_RGBA8888);
	const auto *src = reinterpret_cast<const float *>(data);
	for (int y = 0; y < height; ++y) {
		uchar *dst = img.scanLine(y);
		for (int x = 0; x < width; ++x) {
			for (int c = 0; c < 4; ++c) {
				float v = src[(y * width + x) * 4 + c];
				if (v < 0.0f)
					v = 0.0f;
				if (v > 1.0f)
					v = 1.0f;
				dst[(x * 4) + c] = static_cast<uchar>(v * 255.0f);
			}
		}
	}
	if (!img.save(path)) {
		std::cerr << "Failed to save " << path.toStdString() << std::endl;
	} else {
		std::cerr << "Saved " << path.toStdString() << std::endl;
	}
}

} // namespace

class RenderWorkerFootageTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();
		ProjectSerializer::Initialize();
		DiskManager::CreateInstance();

		demo_path_ = DemoVideoPath();
		ASSERT_TRUE(QFileInfo::exists(demo_path_))
			<< "demo.mp4 not found at " << demo_path_.toStdString();

		worker_path_ = WorkerBinaryPath();
		ASSERT_TRUE(QFileInfo::exists(worker_path_))
			<< "worker binary not found at " << worker_path_.toStdString();

		ASSERT_TRUE(temp_dir_.isValid());

		// Create a minimal project containing the demo footage.
		CreateProjectFile();
	}

	void TearDown() override
	{
		input_region_.Close();
		output_region_.Close();
		if (worker_.state() != QProcess::NotRunning) {
			worker_.terminate();
			worker_.waitForFinished(5000);
			if (worker_.state() != QProcess::NotRunning) {
				worker_.kill();
				worker_.waitForFinished(5000);
			}
		}
		DiskManager::DestroyInstance();
		ProjectSerializer::Destroy();
	}

	void CreateProjectFile()
	{
		project_ = std::make_unique<Project>();
		project_->Initialize();

		footage_ = new Footage(demo_path_);
		footage_->setParent(project_.get());
		footage_->SetLabel(QStringLiteral("demo"));
		ASSERT_TRUE(footage_->IsValid())
			<< "Footage failed to probe " << demo_path_.toStdString();

		footage_id_ = QString::number(reinterpret_cast<quintptr>(footage_));

		project_file_ = FileFunctions::GetSafeTemporaryFilename(
			temp_dir_.filePath(QStringLiteral("worker_graph.ove")));

		ProjectSerializer::Result r = ProjectSerializer::Save(
			ProjectSerializer::SaveData(ProjectSerializer::kProject,
										project_.get(), project_file_),
			false);
		ASSERT_EQ(r.code(), ProjectSerializer::kSuccess)
			<< "Failed to save project file: " << r.GetDetails().toStdString();
		ASSERT_TRUE(QFileInfo::exists(project_file_));
	}

	bool StartWorker(const QString &backend)
	{
		// ---- decode a frame so we know the dimensions and slot sizes ----
		DecoderPtr decoder = Decoder::CreateFromID(QStringLiteral("ffmpeg"));
		if (!decoder ||
			!decoder->Open(Decoder::CodecStream(demo_path_, 0, nullptr))) {
			return false;
		}
		Decoder::RetrieveVideoParams retrieve;
		retrieve.time = rational(0);
		retrieve.maximum_format = PixelFormat::U16;
		FramePtr frame = decoder->RetrieveVideoFrame(retrieve);
		if (!frame || !frame->is_allocated()) {
			return false;
		}

		input_width_ = frame->width();
		input_height_ = frame->height();
		input_stride_ = frame->linesize_bytes();
		input_bpc_ = VideoParams::GetBytesPerChannel(frame->format());
		input_data_bytes_ = frame->allocated_size();
		decoded_frame_ = frame;

		// Output at 1920x1080 float RGBA, like the real viewer path.
		output_width_ = 1920;
		output_height_ = 1080;
		output_data_bytes_ = size_t(output_width_) * output_height_ * 4 *
							 VideoParams::GetBytesPerChannel(PixelFormat::F32);

		// ---- create shared memory pools ----
		const qint64 owner_pid = QCoreApplication::applicationPid();
		output_shm_key_ = ipc::SharedMemoryRegion::MakeKey(owner_pid, 0);
		input_shm_key_ = ipc::SharedMemoryRegion::MakeKey(owner_pid, 1);

		const size_t output_bytes =
			ipc::FrameSlotPool::BytesNeeded(kOutputSlots, output_data_bytes_);
		const size_t input_bytes =
			ipc::FrameSlotPool::BytesNeeded(kInputSlots, input_data_bytes_);

		if (!output_region_.Open(output_shm_key_, output_bytes,
								 ipc::SharedMemoryRegion::kCreate)) {
			return false;
		}
		if (!input_region_.Open(input_shm_key_, input_bytes,
								ipc::SharedMemoryRegion::kCreate)) {
			return false;
		}

		output_pool_ = std::make_unique<ipc::FrameSlotPool>(
			ipc::FrameSlotPool::Create(output_region_.data(), kOutputSlots,
									   output_data_bytes_));
		input_pool_ = std::make_unique<ipc::FrameSlotPool>(
			ipc::FrameSlotPool::Create(input_region_.data(), kInputSlots,
									   input_data_bytes_));

		if (!output_pool_->IsValid() || !input_pool_->IsValid()) {
			return false;
		}

		// ---- spawn worker ----
		worker_.setProcessChannelMode(QProcess::SeparateChannels);
		worker_.start(worker_path_,
					  QStringList{ QStringLiteral("--backend"), backend });
		if (!worker_.waitForStarted(kTimeoutMs)) {
			return false;
		}

		// ---- wait for worker handshake ----
		if (!WaitForMessage(&worker_handshake_)) {
			return false;
		}
		if (worker_handshake_[QStringLiteral("type")].toString() !=
			QLatin1String(ipc::msgtype::kHandshake)) {
			return false;
		}

		// ---- respond with our shm keys ----
		ipc::HandshakeMsg response;
		response.protocol_version = 1;
		response.shm_key = output_shm_key_;
		response.input_shm_key = input_shm_key_;
		response.input_slots = kInputSlots;
		response.output_slots = kOutputSlots;
		response.slot_data_bytes = qint64(output_data_bytes_);
		response.input_slot_data_bytes = qint64(input_data_bytes_);
		if (!ipc::WriteMessage(&worker_, response.ToJson())) {
			return false;
		}

		// ---- load graph ----
		ipc::LoadGraphMsg load;
		load.path = project_file_;
		if (!ipc::WriteMessage(&worker_, load.ToJson())) {
			return false;
		}

		// ---- wait for graph_loaded ----
		QJsonObject loaded;
		if (!WaitForMessage(&loaded)) {
			return false;
		}
		if (loaded[QStringLiteral("type")].toString() !=
			QLatin1String("graph_loaded")) {
			return false;
		}

		return true;
	}

	bool RenderFrameAndWait(int *output_slot)
	{
		// Publish the decoded frame to the input pool. The worker consumes it and
		// releases it back, so we re-publish before every render.
		uint32_t input_slot = 0;
		if (!input_pool_->Acquire(&input_slot)) {
			return false;
		}
		std::memcpy(input_pool_->SlotData(input_slot),
					decoded_frame_->const_data(), input_data_bytes_);
		ipc::FrameSlotMeta *meta = input_pool_->Meta(input_slot);
		meta->id = 0;
		meta->time_num = 0;
		meta->time_den = 1;
		meta->width = input_width_;
		meta->height = input_height_;
		meta->format = int32_t(decoded_frame_->format());
		meta->channel_count = decoded_frame_->channel_count();
		meta->linesize = input_stride_;
		meta->data_size = int32_t(input_data_bytes_);
		std::strncpy(
			meta->colorspace,
			decoded_frame_->video_params().colorspace().toUtf8().constData(),
			sizeof(meta->colorspace) - 1);
		meta->colorspace[sizeof(meta->colorspace) - 1] = '\0';
		input_pool_->Publish(input_slot);

		ipc::RenderFrameMsg req;
		req.ticket_id = 1;
		req.node_uuid = footage_id_;
		req.time_num = 0;
		req.time_den = 1;
		req.width = output_width_;
		req.height = output_height_;
		req.format = int(PixelFormat::F32);
		req.channel_count = VideoParams::kRGBAChannelCount;
		req.mode = int(RenderMode::kOnline);
		req.input_slot = 0;
		if (!ipc::WriteMessage(&worker_, req.ToJson())) {
			std::cerr << "RenderFrameAndWait: failed to write request"
					  << std::endl;
			return false;
		}

		QJsonObject ready;
		if (!WaitForMessage(&ready)) {
			std::cerr << "RenderFrameAndWait: failed to receive ready message"
					  << std::endl;
			return false;
		}
		if (ready[QStringLiteral("type")].toString() !=
			QLatin1String(ipc::msgtype::kFrameReady)) {
			std::cerr << "RenderFrameAndWait: unexpected message type "
					  << ready[QStringLiteral("type")].toString().toStdString()
					  << " body="
					  << QJsonDocument(ready)
							 .toJson(QJsonDocument::Compact)
							 .toStdString()
					  << std::endl;
			return false;
		}
		*output_slot = ready[QStringLiteral("slot")].toInt();
		return true;
	}

	bool WaitForMessage(QJsonObject *out)
	{
		QElapsedTimer timer;
		timer.start();
		while (!timer.hasExpired(kTimeoutMs)) {
			if (worker_.waitForReadyRead(100)) {
				read_buffer_.append(worker_.readAllStandardOutput());
			}
			bool ok = true;
			if (ipc::ReadMessage(&read_buffer_, out, &ok)) {
				return true;
			}
			if (!ok) {
				std::cerr << "WaitForMessage: parse error, buffer="
						  << read_buffer_.toStdString() << std::endl;
				return false;
			}
			if (worker_.state() == QProcess::NotRunning) {
				std::cerr << "WaitForMessage: worker exited with code "
						  << worker_.exitCode() << std::endl;
				std::cerr
					<< "Worker stdout buffer: " << read_buffer_.toStdString()
					<< std::endl;
				QByteArray err = worker_.readAllStandardError();
				if (!err.isEmpty()) {
					std::cerr << "Worker stderr:\n"
							  << err.toStdString() << std::endl;
				}
				return false;
			}
		}
		std::cerr
			<< "WaitForMessage: timeout, buffer=" << read_buffer_.toStdString()
			<< std::endl;
		return false;
	}

	QString demo_path_;
	QString worker_path_;
	QString project_file_;
	QString footage_id_;
	QString output_shm_key_;
	QString input_shm_key_;
	QTemporaryDir temp_dir_;

	std::unique_ptr<Project> project_;
	Footage *footage_ = nullptr;

	QProcess worker_;
	QJsonObject worker_handshake_;
	QByteArray read_buffer_;

	ipc::SharedMemoryRegion output_region_;
	ipc::SharedMemoryRegion input_region_;
	std::unique_ptr<ipc::FrameSlotPool> output_pool_;
	std::unique_ptr<ipc::FrameSlotPool> input_pool_;

	FramePtr decoded_frame_;

	int input_width_ = 0;
	int input_height_ = 0;
	int input_stride_ = 0;
	int input_bpc_ = 0;
	size_t input_data_bytes_ = 0;

	int output_width_ = 0;
	int output_height_ = 0;
	size_t output_data_bytes_ = 0;
};

TEST_F(RenderWorkerFootageTest, VulkanFootageIsNotBlack)
{
	if (!IsRenderBackendAvailable(QStringLiteral("vulkan"))) {
		GTEST_SKIP() << "Vulkan backend is not available in this environment";
	}

	ASSERT_TRUE(StartWorker(QStringLiteral("vulkan")));

	int output_slot = -1;
	ASSERT_TRUE(RenderFrameAndWait(&output_slot));
	ASSERT_GE(output_slot, 0);
	ASSERT_LT(output_slot, kOutputSlots);

	uint32_t consumed_slot = 0;
	ASSERT_TRUE(output_pool_->Consume(&consumed_slot));
	ASSERT_EQ(int(consumed_slot), output_slot);

	const void *output_data = output_pool_->SlotData(consumed_slot);
	const double brightness =
		SampleBrightnessF32(output_data, output_width_, output_height_,
							output_width_ * 4 * int(sizeof(float)));

	EXPECT_GT(brightness, 0.01)
		<< "Worker output frame is black (brightness=" << brightness << ")";

	SaveFrameAsPng(
		output_data, output_width_, output_height_,
		temp_dir_.filePath(QStringLiteral("worker_output_vulkan.png")));
	QFile::remove(QStringLiteral("/tmp/worker_output_vulkan.png"));
	QFile::copy(temp_dir_.filePath(QStringLiteral("worker_output_vulkan.png")),
				QStringLiteral("/tmp/worker_output_vulkan.png"));
	std::cerr << "Vulkan output copied to /tmp/worker_output_vulkan.png"
			  << std::endl;
	output_pool_->Release(consumed_slot);
}

TEST_F(RenderWorkerFootageTest, OpenGLFootageIsNotBlack)
{
	if (!IsRenderBackendAvailable(QStringLiteral("opengl"))) {
		GTEST_SKIP() << "OpenGL backend is not available in this environment";
	}

	ASSERT_TRUE(StartWorker(QStringLiteral("opengl")));

	int output_slot = -1;
	ASSERT_TRUE(RenderFrameAndWait(&output_slot));
	ASSERT_GE(output_slot, 0);
	ASSERT_LT(output_slot, kOutputSlots);

	uint32_t consumed_slot = 0;
	ASSERT_TRUE(output_pool_->Consume(&consumed_slot));
	ASSERT_EQ(int(consumed_slot), output_slot);

	const void *output_data = output_pool_->SlotData(consumed_slot);
	const double brightness =
		SampleBrightnessF32(output_data, output_width_, output_height_,
							output_width_ * 4 * int(sizeof(float)));

	EXPECT_GT(brightness, 0.01)
		<< "Worker output frame is black (brightness=" << brightness << ")";

	SaveFrameAsPng(
		output_data, output_width_, output_height_,
		temp_dir_.filePath(QStringLiteral("worker_output_opengl.png")));
	QFile::remove(QStringLiteral("/tmp/worker_output_opengl.png"));
	QFile::copy(temp_dir_.filePath(QStringLiteral("worker_output_opengl.png")),
				QStringLiteral("/tmp/worker_output_opengl.png"));
	std::cerr << "OpenGL output copied to /tmp/worker_output_opengl.png"
			  << std::endl;
	output_pool_->Release(consumed_slot);
}
