/*
 * Oak Video Editor - AudioManager + ViewerOutput headless tests
 *
 * Covers the CPU-safe surface of olive::AudioManager (PortAudio device
 * bookkeeping that never opens a stream) and olive::ViewerOutput (parameter
 * setters/getters, stream arrays, connected-output resolution, signals and
 * XML (de)serialization). Anything that requires an actual audio output or
 * input stream is intentionally not exercised here.
 */

#include <gtest/gtest.h>

#include <memory>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "audio/audiomanager.h"
#include "node/block/clip/clip.h"
#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/globals.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/sampleformat.h"

// ============================================================================
// AudioManager (no audio hardware required: no stream is ever opened)
// ============================================================================

class AudioManagerTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		olive::AudioManager::CreateInstance();
		ASSERT_NE(olive::AudioManager::instance(), nullptr);
	}

	void TearDown() override
	{
		olive::AudioManager::DestroyInstance();
		EXPECT_EQ(olive::AudioManager::instance(), nullptr);
	}
};

TEST_F(AudioManagerTest, InstanceLifecycle)
{
	// The fixture already created the instance; creating again must be a no-op
	olive::AudioManager *first = olive::AudioManager::instance();
	olive::AudioManager::CreateInstance();
	EXPECT_EQ(olive::AudioManager::instance(), first);

	// Whatever PortAudio reports, the stored indices are either a valid
	// device index or paNoDevice
	EXPECT_GE(olive::AudioManager::instance()->GetOutputDevice(), paNoDevice);
	EXPECT_GE(olive::AudioManager::instance()->GetInputDevice(), paNoDevice);
}

TEST_F(AudioManagerTest, SetAndGetNoDevice)
{
	olive::AudioManager::instance()->SetOutputDevice(paNoDevice);
	EXPECT_EQ(olive::AudioManager::instance()->GetOutputDevice(), paNoDevice);

	olive::AudioManager::instance()->SetInputDevice(paNoDevice);
	EXPECT_EQ(olive::AudioManager::instance()->GetInputDevice(), paNoDevice);
}

TEST_F(AudioManagerTest, PushToOutputWithoutDeviceFails)
{
	olive::AudioManager::instance()->SetOutputDevice(paNoDevice);

	const olive::core::AudioParams params(
		48000, olive::core::kChannelLayoutStereo,
		olive::core::SampleFormat::F32P);
	const QByteArray samples(1024, 0);

	QString error;
	EXPECT_FALSE(
		olive::AudioManager::instance()->PushToOutput(params, samples, &error));
	EXPECT_EQ(error, QStringLiteral("No output device is set"));

	// A null error pointer must be tolerated too
	EXPECT_FALSE(
		olive::AudioManager::instance()->PushToOutput(params, samples, nullptr));
}

TEST_F(AudioManagerTest, StartRecordingWithoutInputDeviceFails)
{
	olive::AudioManager::instance()->SetInputDevice(paNoDevice);

	// Fails before any encoder or PortAudio stream is created
	QString error;
	EXPECT_FALSE(
		olive::AudioManager::instance()->StartRecording(olive::EncodingParams(),
														&error));

	// Tearing down a recording that never started must be harmless
	olive::AudioManager::instance()->StopRecording();
}

TEST_F(AudioManagerTest, OutputControlsWithoutStreamAreNoOps)
{
	// No output stream is open; all of these must be harmless no-ops
	olive::AudioManager::instance()->StopOutput();
	olive::AudioManager::instance()->ClearBufferedOutput();
	olive::AudioManager::instance()->SetOutputNotifyInterval(64);
	olive::AudioManager::instance()->SetOutputNotifyInterval(0);

	SUCCEED();
}

TEST_F(AudioManagerTest, HardResetKeepsManagerUsable)
{
	olive::AudioManager::instance()->HardReset();

	// Device bookkeeping must survive a PortAudio terminate/init cycle
	olive::AudioManager::instance()->SetOutputDevice(paNoDevice);
	EXPECT_EQ(olive::AudioManager::instance()->GetOutputDevice(), paNoDevice);
}

TEST_F(AudioManagerTest, FindDeviceByNameReturnsValidIndexOrNoDevice)
{
	const PaDeviceIndex bogus_output = olive::AudioManager::FindDeviceByName(
		QStringLiteral("OakNoSuchAudioDevice12345"), true);
	EXPECT_TRUE(bogus_output == paNoDevice ||
				(bogus_output >= 0 && bogus_output < Pa_GetDeviceCount()));

	const PaDeviceIndex bogus_input = olive::AudioManager::FindDeviceByName(
		QStringLiteral("OakNoSuchAudioDevice12345"), false);
	EXPECT_TRUE(bogus_input == paNoDevice ||
				(bogus_input >= 0 && bogus_input < Pa_GetDeviceCount()));

	// An empty name falls back to the default/preferred device
	const PaDeviceIndex fallback =
		olive::AudioManager::FindDeviceByName(QString(), true);
	EXPECT_TRUE(fallback == paNoDevice ||
				(fallback >= 0 && fallback < Pa_GetDeviceCount()));
}

TEST_F(AudioManagerTest, FindConfigDeviceByNameReturnsValidIndexOrNoDevice)
{
	const PaDeviceIndex output =
		olive::AudioManager::FindConfigDeviceByName(true);
	EXPECT_TRUE(output == paNoDevice ||
				(output >= 0 && output < Pa_GetDeviceCount()));

	const PaDeviceIndex input =
		olive::AudioManager::FindConfigDeviceByName(false);
	EXPECT_TRUE(input == paNoDevice ||
				(input >= 0 && input < Pa_GetDeviceCount()));
}

TEST_F(AudioManagerTest, PortAudioParamsReflectAudioParams)
{
	if (Pa_GetDeviceCount() <= 0) {
		GTEST_SKIP() << "No PortAudio devices available on this system";
	}

	const olive::core::AudioParams params(
		48000, olive::core::kChannelLayoutStereo,
		olive::core::SampleFormat::F32);
	const PaStreamParameters p =
		olive::AudioManager::GetPortAudioParams(params, 0);

	EXPECT_EQ(p.channelCount, 2);
	EXPECT_EQ(p.device, 0);
	EXPECT_EQ(p.sampleFormat, PaSampleFormat(paFloat32));
	EXPECT_EQ(p.hostApiSpecificStreamInfo, nullptr);
	EXPECT_GE(p.suggestedLatency, 0.0);
}

TEST_F(AudioManagerTest, PortAudioParamsMapsSampleFormats)
{
	if (Pa_GetDeviceCount() <= 0) {
		GTEST_SKIP() << "No PortAudio devices available on this system";
	}

	const auto format_for = [](olive::core::SampleFormat f) {
		const olive::core::AudioParams params(
			48000, olive::core::kChannelLayoutMono, f);
		return olive::AudioManager::GetPortAudioParams(params, 0).sampleFormat;
	};

	// Packed and planar variants of the same depth map to the same flag
	EXPECT_EQ(format_for(olive::core::SampleFormat::U8),
			  PaSampleFormat(paUInt8));
	EXPECT_EQ(format_for(olive::core::SampleFormat::U8P),
			  PaSampleFormat(paUInt8));
	EXPECT_EQ(format_for(olive::core::SampleFormat::S16),
			  PaSampleFormat(paInt16));
	EXPECT_EQ(format_for(olive::core::SampleFormat::S16P),
			  PaSampleFormat(paInt16));
	EXPECT_EQ(format_for(olive::core::SampleFormat::S32),
			  PaSampleFormat(paInt32));
	EXPECT_EQ(format_for(olive::core::SampleFormat::S32P),
			  PaSampleFormat(paInt32));
	EXPECT_EQ(format_for(olive::core::SampleFormat::F32),
			  PaSampleFormat(paFloat32));
	EXPECT_EQ(format_for(olive::core::SampleFormat::F32P),
			  PaSampleFormat(paFloat32));

	// 64-bit depths have no PortAudio equivalent and map to paCustomFormat(0)
	EXPECT_EQ(format_for(olive::core::SampleFormat::S64), PaSampleFormat(0));
	EXPECT_EQ(format_for(olive::core::SampleFormat::F64), PaSampleFormat(0));
	EXPECT_EQ(format_for(olive::core::SampleFormat::INVALID),
			  PaSampleFormat(0));
}

// ============================================================================
// ViewerOutput
// ============================================================================

// AddStream/SetStream are protected on ViewerOutput (Sequence is the intended
// caller); expose them so the stream-array bookkeeping can be tested directly
class TestViewerOutput : public olive::ViewerOutput {
public:
	using olive::ViewerOutput::ViewerOutput;
	using olive::ViewerOutput::AddStream;
	using olive::ViewerOutput::SetStream;
};

class ViewerOutputTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		olive::ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();

		viewer_ = new TestViewerOutput();
		viewer_->setParent(project_.get());
	}

	template <typename T> T *AddNode()
	{
		T *node = new T();
		node->setParent(project_.get());
		return node;
	}

	std::unique_ptr<olive::Project> project_;
	TestViewerOutput *viewer_;
};

TEST_F(ViewerOutputTest, DefaultConstruction)
{
	EXPECT_EQ(viewer_->Name(), QStringLiteral("Viewer"));
	EXPECT_EQ(viewer_->id(),
			  QStringLiteral("org.olivevideoeditor.Olive.vieweroutput"));
	EXPECT_TRUE(viewer_->Category().contains(olive::Node::kCategoryOutput));

	// One default video and audio stream, no subtitle streams
	EXPECT_EQ(viewer_->GetVideoStreamCount(), 1);
	EXPECT_EQ(viewer_->GetAudioStreamCount(), 1);
	EXPECT_EQ(viewer_->GetSubtitleStreamCount(), 0);
	EXPECT_EQ(viewer_->GetTotalStreamCount(), 2);

	EXPECT_NE(viewer_->GetWorkArea(), nullptr);
	EXPECT_NE(viewer_->GetMarkers(), nullptr);

	EXPECT_EQ(viewer_->GetPlayhead(), olive::rational(0));
	EXPECT_EQ(viewer_->GetLength(), olive::rational(0));
	EXPECT_EQ(viewer_->GetVideoLength(), olive::rational(0));
	EXPECT_EQ(viewer_->GetAudioLength(), olive::rational(0));

	EXPECT_EQ(viewer_->GetConnectedTextureOutput(), nullptr);
	EXPECT_EQ(viewer_->GetConnectedSampleOutput(), nullptr);
	EXPECT_EQ(viewer_->GetConnectedWaveform(), nullptr);

	// The autocache API is currently a stub that always reports disabled
	EXPECT_FALSE(viewer_->IsVideoAutoCacheEnabled());
}

TEST_F(ViewerOutputTest, SetAndGetVideoParams)
{
	const olive::VideoParams vp(1920, 1080, olive::rational(1, 30),
								olive::PixelFormat::U8, 4);
	viewer_->SetVideoParams(vp);

	EXPECT_EQ(viewer_->GetVideoParams(), vp);
	EXPECT_EQ(viewer_->GetVideoParams().width(), 1920);
	EXPECT_EQ(viewer_->GetVideoParams().height(), 1080);

	// Out-of-range indices return invalid params instead of garbage
	EXPECT_FALSE(viewer_->GetVideoParams(5).is_valid());
}

TEST_F(ViewerOutputTest, SetAndGetAudioParams)
{
	const olive::core::AudioParams ap(48000, olive::core::kChannelLayoutStereo,
									  olive::core::SampleFormat::F32P);
	viewer_->SetAudioParams(ap);

	EXPECT_EQ(viewer_->GetAudioParams(), ap);

	// The default-constructed stream uses the viewer's default sample format
	const olive::core::SampleFormat default_format =
		olive::ViewerOutput::kDefaultSampleFormat;
	EXPECT_EQ(viewer_->GetAudioParams().format(), default_format);

	// Out-of-range indices return invalid params instead of garbage
	EXPECT_FALSE(viewer_->GetAudioParams(9).is_valid());
}

TEST_F(ViewerOutputTest, SetAndGetSubtitleParams)
{
	olive::SubtitleParams subs;
	subs.push_back(olive::Subtitle(
		olive::TimeRange(olive::rational(0), olive::rational(2)),
		QStringLiteral("hello")));

	EXPECT_EQ(viewer_->AddStream(olive::Track::kSubtitle,
								 QVariant::fromValue(subs)),
			  0);
	EXPECT_EQ(viewer_->GetSubtitleStreamCount(), 1);

	ASSERT_TRUE(viewer_->GetSubtitleParams(0).is_valid());
	EXPECT_EQ(viewer_->GetSubtitleParams(0).duration(), olive::rational(2));
	EXPECT_TRUE(viewer_->HasEnabledSubtitleStreams());

	// Out-of-range indices return invalid (empty) params
	EXPECT_FALSE(viewer_->GetSubtitleParams(3).is_valid());
}

TEST_F(ViewerOutputTest, VideoParamSignals)
{
	int size_emissions = 0;
	int frame_rate_emissions = 0;
	int pixel_aspect_emissions = 0;
	int interlacing_emissions = 0;
	int params_emissions = 0;
	olive::rational emitted_frame_rate;
	QObject::connect(viewer_, &olive::ViewerOutput::SizeChanged,
					 [&size_emissions](int, int) { ++size_emissions; });
	QObject::connect(viewer_, &olive::ViewerOutput::FrameRateChanged,
					 [&frame_rate_emissions, &emitted_frame_rate](
						 const olive::rational &r) {
						 ++frame_rate_emissions;
						 emitted_frame_rate = r;
					 });
	QObject::connect(viewer_, &olive::ViewerOutput::PixelAspectChanged,
					 [&pixel_aspect_emissions](const olive::rational &) {
						 ++pixel_aspect_emissions;
					 });
	QObject::connect(viewer_, &olive::ViewerOutput::InterlacingChanged,
					 [&interlacing_emissions](olive::VideoParams::Interlacing) {
						 ++interlacing_emissions;
					 });
	QObject::connect(viewer_, &olive::ViewerOutput::VideoParamsChanged,
					 [&params_emissions]() { ++params_emissions; });

	// Every aspect differs from the cached defaults, so all signals fire once
	const olive::VideoParams vp(1280, 720, olive::rational(1, 60),
								olive::PixelFormat::U8, 4, olive::rational(2),
								olive::VideoParams::kInterlacedTopFirst);
	viewer_->SetVideoParams(vp);

	EXPECT_EQ(size_emissions, 1);
	EXPECT_EQ(frame_rate_emissions, 1);
	EXPECT_EQ(emitted_frame_rate, olive::rational(60, 1));
	EXPECT_EQ(pixel_aspect_emissions, 1);
	EXPECT_EQ(interlacing_emissions, 1);
	EXPECT_EQ(params_emissions, 1);

	// Setting identical params only re-emits the unconditional change signal
	viewer_->SetVideoParams(vp);

	EXPECT_EQ(size_emissions, 1);
	EXPECT_EQ(frame_rate_emissions, 1);
	EXPECT_EQ(pixel_aspect_emissions, 1);
	EXPECT_EQ(interlacing_emissions, 1);
	EXPECT_EQ(params_emissions, 2);
}

TEST_F(ViewerOutputTest, AudioParamSignals)
{
	int sample_rate_emissions = 0;
	int params_emissions = 0;
	int emitted_sample_rate = 0;
	QObject::connect(viewer_, &olive::ViewerOutput::SampleRateChanged,
					 [&sample_rate_emissions, &emitted_sample_rate](int sr) {
						 ++sample_rate_emissions;
						 emitted_sample_rate = sr;
					 });
	QObject::connect(viewer_, &olive::ViewerOutput::AudioParamsChanged,
					 [&params_emissions]() { ++params_emissions; });

	const olive::core::AudioParams ap(44100, olive::core::kChannelLayoutStereo,
									  olive::core::SampleFormat::F32P);
	viewer_->SetAudioParams(ap);

	EXPECT_EQ(sample_rate_emissions, 1);
	EXPECT_EQ(emitted_sample_rate, 44100);
	EXPECT_EQ(params_emissions, 1);

	// Same sample rate again: no SampleRateChanged, but AudioParamsChanged
	viewer_->SetAudioParams(ap);

	EXPECT_EQ(sample_rate_emissions, 1);
	EXPECT_EQ(params_emissions, 2);
}

TEST_F(ViewerOutputTest, SetPlayheadEmitsPlayheadChanged)
{
	int emissions = 0;
	olive::rational emitted;
	QObject::connect(viewer_, &olive::ViewerOutput::PlayheadChanged,
					 [&emissions, &emitted](const olive::rational &t) {
						 ++emissions;
						 emitted = t;
					 });

	viewer_->SetPlayhead(olive::rational(3, 2));

	EXPECT_EQ(emissions, 1);
	EXPECT_EQ(emitted, olive::rational(3, 2));
	EXPECT_EQ(viewer_->GetPlayhead(), olive::rational(3, 2));
}

TEST_F(ViewerOutputTest, VerifyLengthWithoutConnectionsStaysZero)
{
	int length_emissions = 0;
	QObject::connect(viewer_, &olive::ViewerOutput::LengthChanged,
					 [&length_emissions](const olive::rational &) {
						 ++length_emissions;
					 });

	viewer_->VerifyLength();
	viewer_->VerifyLength();

	// Nothing is connected, so all lengths stay zero and nothing is emitted
	EXPECT_EQ(viewer_->GetLength(), olive::rational(0));
	EXPECT_EQ(viewer_->GetVideoLength(), olive::rational(0));
	EXPECT_EQ(viewer_->GetAudioLength(), olive::rational(0));
	EXPECT_EQ(length_emissions, 0);

	EXPECT_EQ(viewer_->GetVideoCacheRange(),
			  olive::TimeRange(olive::rational(0), olive::rational(0)));
	EXPECT_EQ(viewer_->GetAudioCacheRange(),
			  olive::TimeRange(olive::rational(0), olive::rational(0)));
}

TEST_F(ViewerOutputTest, StreamEnableDisable)
{
	ASSERT_TRUE(viewer_->HasEnabledVideoStreams());
	ASSERT_TRUE(viewer_->HasEnabledAudioStreams());

	olive::VideoParams vp = viewer_->GetVideoParams();
	vp.set_enabled(false);
	viewer_->SetVideoParams(vp);

	EXPECT_FALSE(viewer_->HasEnabledVideoStreams());
	EXPECT_FALSE(viewer_->GetFirstEnabledVideoStream().is_valid());
	EXPECT_TRUE(viewer_->GetEnabledVideoStreams().isEmpty());

	olive::core::AudioParams ap = viewer_->GetAudioParams();
	ap.set_enabled(false);
	viewer_->SetAudioParams(ap);

	EXPECT_FALSE(viewer_->HasEnabledAudioStreams());
	EXPECT_FALSE(viewer_->GetFirstEnabledAudioStream().is_valid());
	EXPECT_TRUE(viewer_->GetEnabledAudioStreams().isEmpty());
	EXPECT_TRUE(viewer_->GetEnabledStreamsAsReferences().isEmpty());
}

TEST_F(ViewerOutputTest, AddAndSetStreams)
{
	const olive::VideoParams vp2(640, 360, olive::rational(1, 25),
								 olive::PixelFormat::U8, 4);

	EXPECT_EQ(viewer_->AddStream(olive::Track::kVideo,
								 QVariant::fromValue(vp2)),
			  1);
	EXPECT_EQ(viewer_->GetVideoStreamCount(), 2);
	EXPECT_EQ(viewer_->GetVideoParams(1), vp2);
	EXPECT_EQ(viewer_->GetTotalStreamCount(), 3);

	// References enumerate the enabled streams in video/audio/subtitle order
	const QVector<olive::Track::Reference> refs =
		viewer_->GetEnabledStreamsAsReferences();
	ASSERT_EQ(refs.size(), 3);
	EXPECT_EQ(refs.at(0), olive::Track::Reference(olive::Track::kVideo, 0));
	EXPECT_EQ(refs.at(1), olive::Track::Reference(olive::Track::kVideo, 1));
	EXPECT_EQ(refs.at(2), olive::Track::Reference(olive::Track::kAudio, 0));

	// SetStream replaces an existing element in place
	const olive::core::AudioParams ap2(
		32000, olive::core::kChannelLayoutMono, olive::core::SampleFormat::S16);
	EXPECT_EQ(viewer_->SetStream(olive::Track::kAudio,
								 QVariant::fromValue(ap2), 0),
			  0);
	EXPECT_EQ(viewer_->GetAudioStreamCount(), 1);
	EXPECT_EQ(viewer_->GetAudioParams(0), ap2);

	// kNone is not a valid stream type
	EXPECT_EQ(viewer_->AddStream(olive::Track::kNone, QVariant()), -1);
}

TEST_F(ViewerOutputTest, ConnectTextureEmitsAndResolves)
{
	auto *solid = AddNode<olive::SolidGenerator>();

	int emissions = 0;
	QObject::connect(viewer_, &olive::ViewerOutput::TextureInputChanged,
					 [&emissions]() { ++emissions; });

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(viewer_, olive::ViewerOutput::kTextureInput));

	EXPECT_EQ(emissions, 1);
	EXPECT_EQ(viewer_->GetConnectedTextureOutput(), solid);

	// Explicit value hints set on the input are reported through the getter
	const olive::Node::ValueHint hint({ olive::NodeValue::kTexture }, 2,
									  QStringLiteral("tex"));
	viewer_->SetValueHintForInput(olive::ViewerOutput::kTextureInput, hint);
	const olive::Node::ValueHint got =
		viewer_->GetConnectedTextureValueHint();
	ASSERT_EQ(got.types().size(), 1);
	EXPECT_EQ(got.types().first(), olive::NodeValue::kTexture);
	EXPECT_EQ(got.index(), 2);
	EXPECT_EQ(got.tag(), QStringLiteral("tex"));

	olive::Node::DisconnectEdge(
		solid, olive::NodeInput(viewer_, olive::ViewerOutput::kTextureInput));

	EXPECT_EQ(emissions, 2);
	EXPECT_EQ(viewer_->GetConnectedTextureOutput(), nullptr);
}

TEST_F(ViewerOutputTest, ConnectSamplesResolves)
{
	auto *clip = AddNode<olive::ClipBlock>();

	olive::Node::ConnectEdge(
		clip, olive::NodeInput(viewer_, olive::ViewerOutput::kSamplesInput));

	EXPECT_EQ(viewer_->GetConnectedSampleOutput(), clip);
	EXPECT_EQ(viewer_->GetConnectedWaveform(), clip->waveform_cache());

	// Without an explicit hint the sample value hint is empty
	EXPECT_TRUE(viewer_->GetConnectedSampleValueHint().types().isEmpty());

	olive::Node::DisconnectEdge(
		clip, olive::NodeInput(viewer_, olive::ViewerOutput::kSamplesInput));

	EXPECT_EQ(viewer_->GetConnectedSampleOutput(), nullptr);
	EXPECT_EQ(viewer_->GetConnectedWaveform(), nullptr);
}

TEST_F(ViewerOutputTest, InvalidateCacheWithoutConnectionsIsSafe)
{
	// With nothing connected the request path is skipped entirely; this must
	// neither crash nor produce a length change
	viewer_->InvalidateCache(olive::TimeRange(olive::rational(0),
											  olive::rational(1)),
							 olive::ViewerOutput::kTextureInput, -1,
							 olive::Node::InvalidateCacheOptions());
	viewer_->InvalidateCache(olive::TimeRange(olive::rational(0),
											  olive::rational(1)),
							 olive::ViewerOutput::kSamplesInput, -1,
							 olive::Node::InvalidateCacheOptions());

	EXPECT_EQ(viewer_->GetLength(), olive::rational(0));
}

TEST_F(ViewerOutputTest, ValueRepushTagsStreams)
{
	olive::NodeValueRow row;
	row.insert(olive::ViewerOutput::kTextureInput,
			   olive::NodeValue(olive::NodeValue::kTexture, 0));
	row.insert(olive::ViewerOutput::kSamplesInput,
			   olive::NodeValue(olive::NodeValue::kSamples, 0));

	olive::NodeValueTable table;
	viewer_->Value(row, olive::NodeGlobals(), &table);

	// The texture value is re-pushed tagged as video stream 0
	const QString video_tag =
		olive::Track::Reference(olive::Track::kVideo, 0).ToString();
	EXPECT_EQ(table.Get(olive::NodeValue::kTexture, video_tag).type(),
			  olive::NodeValue::kTexture);

	// The samples value is re-pushed tagged as audio stream 0
	const QString audio_tag =
		olive::Track::Reference(olive::Track::kAudio, 0).ToString();
	EXPECT_EQ(table.Get(olive::NodeValue::kSamples, audio_tag).type(),
			  olive::NodeValue::kSamples);
	EXPECT_EQ(table.Get(olive::NodeValue::kSamples).tag(), audio_tag);
}

TEST_F(ViewerOutputTest, LastUsedEncodingParamsRoundTrip)
{
	olive::EncodingParams params;
	params.SetFilename(QStringLiteral("/tmp/oak-export.mp4"));

	viewer_->SetLastUsedEncodingParams(params);

	EXPECT_EQ(viewer_->GetLastUsedEncodingParams().filename(),
			  QStringLiteral("/tmp/oak-export.mp4"));
}

TEST_F(ViewerOutputTest, SaveLoadCustomRoundTrip)
{
	viewer_->GetWorkArea()->set_enabled(true);
	viewer_->GetWorkArea()->set_range(
		olive::TimeRange(olive::rational(1), olive::rational(5)));

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	viewer_->SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	EXPECT_TRUE(xml.contains(QStringLiteral("workarea")));
	EXPECT_TRUE(xml.contains(QStringLiteral("markers")));

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));

	olive::ViewerOutput loaded;
	ASSERT_TRUE(loaded.LoadCustom(&reader, nullptr));
	EXPECT_TRUE(loaded.GetWorkArea()->enabled());
	EXPECT_EQ(loaded.GetWorkArea()->range(), viewer_->GetWorkArea()->range());
}

TEST_F(ViewerOutputTest, RetranslateSetsInputNames)
{
	viewer_->Retranslate();

	EXPECT_EQ(viewer_->GetInputName(olive::ViewerOutput::kVideoParamsInput),
			  QStringLiteral("Video Parameters"));
	EXPECT_EQ(viewer_->GetInputName(olive::ViewerOutput::kAudioParamsInput),
			  QStringLiteral("Audio Parameters"));
	EXPECT_EQ(viewer_->GetInputName(olive::ViewerOutput::kSubtitleParamsInput),
			  QStringLiteral("Subtitle Parameters"));
	EXPECT_EQ(viewer_->GetInputName(olive::ViewerOutput::kTextureInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(viewer_->GetInputName(olive::ViewerOutput::kSamplesInput),
			  QStringLiteral("Samples"));
}

TEST_F(ViewerOutputTest, AutoCacheStubsAlwaysReportDisabled)
{
	EXPECT_FALSE(viewer_->IsVideoAutoCacheEnabled());

	// The setter is a stub and must not change the reported state
	viewer_->SetVideoAutoCacheEnabled(true);
	EXPECT_FALSE(viewer_->IsVideoAutoCacheEnabled());
}

TEST_F(ViewerOutputTest, SetWaveformEnabledWithoutConnectionIsSafe)
{
	// No samples input connected: enabling waveform requests must not crash
	viewer_->SetWaveformEnabled(true);
	EXPECT_EQ(viewer_->GetConnectedWaveform(), nullptr);

	viewer_->SetWaveformEnabled(false);
}

TEST_F(ViewerOutputTest, FrequencyRateDataReflectsEnabledStreams)
{
	// A video stream takes priority and is reported as a frame rate
	QString rate = viewer_->data(olive::Node::FREQUENCY_RATE).toString();
	EXPECT_TRUE(rate.endsWith(QStringLiteral(" FPS")));

	// With the video stream disabled, the audio sample rate is reported
	olive::VideoParams vp = viewer_->GetVideoParams();
	vp.set_enabled(false);
	viewer_->SetVideoParams(vp);

	rate = viewer_->data(olive::Node::FREQUENCY_RATE).toString();
	EXPECT_TRUE(rate.endsWith(QStringLiteral(" Hz")));
}
