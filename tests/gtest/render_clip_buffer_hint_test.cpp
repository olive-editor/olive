/*
 * Oak Video Editor - Clip Buffer Value Hint Regression Test
 * Copyright (C) 2026 Oak Team
 *
 * Regression test for a pre-existing black-screen bug: when a footage node
 * was connected directly to a clip's buffer input (no effect node in
 * between), the preview rendered black. The buffer input is declared as
 * NodeValue::kNone, so the traverser had no type information when pulling a
 * value from the connected node's table and fell back to the last value in
 * the table. A footage pushes its video texture first and its audio samples
 * last, so a video clip ended up fed with audio samples and produced no
 * texture at all. The fix makes ClipBlock::GetValueHintForInput prefer the
 * value type matching the clip's track.
 *
 * These tests drive the exact application render path:
 * PreviewAutoCacher -> RenderManager -> RenderWorkerPool -> oak-render-worker.
 */

#include <gtest/gtest.h>

#include <atomic>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QThread>

#include "codec/conformmanager.h"
#include "codec/frame.h"
#include "codec/proxymanager.h"
#include "config/config.h"
#include "node/block/clip/clip.h"
#include "node/color/colormanager/colormanager.h"
#include "node/effect/opacity/opacityeffect.h"
#include "node/factory.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "node/project/serializer/serializer.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/sampleformat.h"
#include "render/diskmanager.h"
#include "render/framemanager.h"
#include "render/previewautocacher.h"
#include "render/rendermanager.h"
#include "render/renderticket.h"
#include "task/taskmanager.h"

#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
#include "render/backend/dynamicrenderer.h"
#include "render/backend/renderbackend_c.h"
#endif

using namespace olive;

namespace
{

QString DemoVideoPath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/demo.mp4"));
}

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

bool IsRenderBackendAvailable(const QString &backend)
{
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
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
#else
	Q_UNUSED(backend)
	return false;
#endif
}

// Returns the number of non-zero bytes sampled from the frame buffer, or -1
// when the frame is invalid.
int CountNonZeroBytes(const FramePtr &frame)
{
	if (!frame || !frame->is_allocated()) {
		return -1;
	}
	const auto *data = reinterpret_cast<const uint8_t *>(frame->const_data());
	const int size = frame->allocated_size();
	int nonzero = 0;
	// Sample across the buffer to keep the check fast.
	const int step = qMax(1, size / 4096);
	for (int i = 0; i < size; i += step) {
		if (data[i] != 0) {
			nonzero++;
		}
	}
	return nonzero;
}

} // namespace

class RenderClipBufferHintTest
	: public ::testing::Test,
	  public ::testing::WithParamInterface<QString> {
protected:
	static void SetUpTestSuite()
	{
		// Mirror Core::Start()'s singleton initialization order (RenderManager
		// is created per-test instead, so that each backend gets a fresh one).
		NodeFactory::Initialize();
		ColorManager::SetUpDefaultConfig();
		TaskManager::CreateInstance();
		ConformManager::CreateInstance();
		ProxyManager::CreateInstance();
		FrameManager::CreateInstance();
		ProjectSerializer::Initialize();
		DiskManager::CreateInstance();

		// Point the worker pool at the built worker binary.
		const QString worker = WorkerBinaryPath();
		if (QFileInfo::exists(worker)) {
			qputenv("OAK_RENDER_WORKER", QFile::encodeName(worker));
		}
	}

	static void TearDownTestSuite()
	{
		DiskManager::DestroyInstance();
		ProjectSerializer::Destroy();
		FrameManager::DestroyInstance();
		ProxyManager::DestroyInstance();
		ConformManager::DestroyInstance();
		TaskManager::DestroyInstance();
		NodeFactory::Destroy();
	}

	void SetUp() override
	{
		backend_ = GetParam();
		if (!IsRenderBackendAvailable(backend_)) {
			GTEST_SKIP() << "Render backend is not available: "
						 << backend_.toStdString();
		}

		const QString worker = WorkerBinaryPath();
		if (!QFileInfo::exists(worker)) {
			GTEST_SKIP() << "worker binary not found at "
						 << worker.toStdString();
		}

		demo_path_ = DemoVideoPath();
		ASSERT_TRUE(QFileInfo::exists(demo_path_));

		Config::Current()[QStringLiteral("GraphicsBackend")] = backend_;

		project_ = std::make_unique<Project>();
		project_->Initialize();

		footage_ = new Footage(demo_path_);
		footage_->setParent(project_.get());
		ASSERT_TRUE(footage_->IsValid())
			<< "Footage failed to probe " << demo_path_.toStdString();
		// The bug requires the footage to provide both a video and an audio
		// stream, so that the video texture is not the last value in the
		// footage's table.
		ASSERT_GE(footage_->GetVideoStreamCount(), 1);
		ASSERT_GE(footage_->GetAudioStreamCount(), 1)
			<< "Test footage must contain an audio stream";

		RenderManager::CreateInstance();
		RenderManager::instance()->GetCacher()->SetProject(project_.get());
	}

	void TearDown() override
	{
		// May be null when SetUp() skipped before creating the instance.
		if (RenderManager::instance()) {
			RenderManager::instance()->GetCacher()->SetProject(nullptr);
			RenderManager::DestroyInstance();
		}
		project_.reset();
	}

	// Builds sequence <- track <- clip <- (optional effect) <- footage and
	// returns the clip. When insert_effect is false the footage is connected
	// directly to the clip's buffer input, which is the black-screen case.
	ClipBlock *BuildVideoClipChain(bool insert_effect)
	{
		sequence_ = new Sequence();
		sequence_->setParent(project_.get());
		sequence_->SetVideoParams(VideoParams(
			1920, 1080, rational(25),
			static_cast<PixelFormat::Format>(
				Config::Current()[QStringLiteral("OfflinePixelFormat")]
					.toInt()),
			VideoParams::kInternalChannelCount, rational(1),
			VideoParams::kInterlaceNone, 1));
		sequence_->SetAudioParams(olive::core::AudioParams(
			48000, olive::core::kChannelLayoutStereo,
			olive::core::SampleFormat::F32P));

		Track *track = new Track();
		track->setParent(project_.get());
		video_track_ = track;
		ClipBlock *clip = new ClipBlock();
		clip->setParent(project_.get());
		clip->set_length_and_media_out(footage_->GetLength());

		Node *buffer_source = footage_;
		if (insert_effect) {
			OpacityEffect *opacity = new OpacityEffect();
			opacity->setParent(project_.get());
			Node::ConnectEdge(footage_,
							  NodeInput(opacity, OpacityEffect::kTextureInput));
			buffer_source = opacity;
		}
		Node::ConnectEdge(buffer_source,
						  NodeInput(clip, ClipBlock::kBufferIn));

		track->AppendBlock(clip);

		// Wire the track into the sequence's video track list (this is what
		// assigns the track its type) and into the sequence's texture output.
		TrackList *track_list = sequence_->track_list(Track::kVideo);
		track_list->ArrayAppend();
		Node::ConnectEdge(
			track, track_list->track_input(track_list->ArraySize() - 1));
		Node::ConnectEdge(track,
						  NodeInput(sequence_, ViewerOutput::kTextureInput));

		return clip;
	}

	// Renders one frame through the application's preview path and returns the
	// resulting CPU frame (nullptr on failure/timeout).
	FramePtr RenderOneFrame(ViewerOutput *viewer, const rational &time)
	{
		RenderTicketPtr ticket =
			RenderManager::instance()->GetCacher()->GetSingleFrame(viewer, time,
																   false);
		if (!ticket) {
			return nullptr;
		}

		std::atomic<bool> finished{ false };
		QObject::connect(ticket.get(), &RenderTicket::Finished,
						 [&finished]() { finished = true; });

		QElapsedTimer timer;
		timer.start();
		while (!finished.load() && !timer.hasExpired(60000)) {
			QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
			QThread::msleep(5);
		}

		if (!finished.load() || !ticket->HasResult()) {
			return nullptr;
		}

		return ticket->Get().value<FramePtr>();
	}

	QString backend_;
	QString demo_path_;
	std::unique_ptr<Project> project_;
	Footage *footage_ = nullptr;
	Sequence *sequence_ = nullptr;
	Track *video_track_ = nullptr;
};

// Footage connected directly to a video clip's buffer input must not produce
// a black frame. Before the fix, the clip pulled the footage's audio samples
// (the last value in the footage's table) instead of its video texture.
TEST_P(RenderClipBufferHintTest, DirectFootageToVideoClipNotBlack)
{
	BuildVideoClipChain(false);

	for (double t : { 0.0, 1.0 }) {
		FramePtr frame = RenderOneFrame(sequence_, rational::fromDouble(t));
		ASSERT_TRUE(frame != nullptr)
			<< "Direct footage->clip render produced no frame at t=" << t;
		ASSERT_TRUE(frame->is_allocated());
		EXPECT_GT(CountNonZeroBytes(frame), 0)
			<< "Direct footage->clip render is BLACK at t=" << t
			<< " (all sampled bytes are zero)";
	}
}

// Control case: an effect node between the footage and the clip always worked,
// because the effect's table contains only the passed-through texture.
TEST_P(RenderClipBufferHintTest, IndirectFootageToVideoClipNotBlack)
{
	BuildVideoClipChain(true);

	for (double t : { 0.0, 1.0 }) {
		FramePtr frame = RenderOneFrame(sequence_, rational::fromDouble(t));
		ASSERT_TRUE(frame != nullptr)
			<< "Indirect footage->opacity->clip render produced no frame at t="
			<< t;
		ASSERT_TRUE(frame->is_allocated());
		EXPECT_GT(CountNonZeroBytes(frame), 0)
			<< "Indirect footage->opacity->clip render is BLACK at t=" << t
			<< " (all sampled bytes are zero)";
	}
}

// Unit-level guard: the buffer input's value hint must follow the clip's
// track type so the traverser pulls the right value from a multi-stream
// source.
TEST_P(RenderClipBufferHintTest, BufferHintFollowsTrackType)
{
	ClipBlock *clip = BuildVideoClipChain(false);

	Node::ValueHint video_hint =
		clip->GetValueHintForInput(ClipBlock::kBufferIn);
	ASSERT_FALSE(video_hint.types().isEmpty());
	EXPECT_TRUE(video_hint.types().contains(NodeValue::kTexture));

	// Move the clip onto an audio track: the hint must prefer samples.
	video_track_->RippleRemoveBlock(clip);

	Track *audio_track = new Track();
	audio_track->setParent(project_.get());
	audio_track->set_type(Track::kAudio);
	audio_track->AppendBlock(clip);

	Node::ValueHint audio_hint =
		clip->GetValueHintForInput(ClipBlock::kBufferIn);
	ASSERT_FALSE(audio_hint.types().isEmpty());
	EXPECT_TRUE(audio_hint.types().contains(NodeValue::kSamples));
}

INSTANTIATE_TEST_SUITE_P(Backends, RenderClipBufferHintTest,
						 ::testing::Values(QStringLiteral("opengl"),
										   QStringLiteral("vulkan")));
