/*
 * Oak Video Editor - Render Processor & Manager CPU Tests
 * Copyright (C) 2026 Oak Team
 *
 * CPU-only, headless coverage for render pipeline pieces that need no GL
 * context, worker process, or audio device:
 * - RenderProcessor  (ticket plumbing: audio render + clamp + waveform,
 *                     video dry-run with a null renderer, cancelled tickets)
 * - RenderManager    (RenderVideoParams/RenderAudioParams default plumbing,
 *                     kDryRunInterval)
 * - RenderJobTracker (job-time tagged range bookkeeping)
 * - SubtitleParams   (ASS header generation, XML save/load round trip)
 * - ManagedColor     (color input/output transform plumbing)
 * - Texture          (dummy textures constructible without a renderer)
 *
 * SpscRingBuffer is intentionally not covered here; render_ipc_test.cpp and
 * render_workerpool_ipc_test.cpp already exercise it thoroughly. The backend
 * string conversions are covered by config_test.cpp.
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>

#include <QMatrix4x4>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QVector2D>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <olive/core/core.h>

#include "audio/audiovisualwaveform.h"
#include "common/jobtime.h"
#include "common/qtutils.h"
#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/globals.h"
#include "node/project.h"
#include "render/job/acceleratedjob.h"
#include "render/managedcolor.h"
#include "render/renderjobtracker.h"
#include "render/rendermanager.h"
#include "render/renderprocessor.h"
#include "render/subtitleparams.h"
#include "render/texture.h"

namespace
{

// A node that emits a constant, deliberately over-range (+2.0) sample buffer
// so RenderProcessor's clamp path has observable work to do.
class ConstantSampleNode : public olive::Node {
public:
	ConstantSampleNode() = default;

	NODE_DEFAULT_FUNCTIONS(ConstantSampleNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Constant Sample Node");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.constant_sample_node");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return {};
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)

		const olive::core::AudioParams &params = globals.aparams();
		const size_t sample_count =
			size_t(params.time_to_samples(globals.time().length()));

		olive::core::SampleBuffer buffer(params, sample_count);
		for (int ch = 0; ch < buffer.channel_count(); ch++) {
			float *data = buffer.data(ch);
			for (size_t i = 0; i < sample_count; i++) {
				data[i] = 2.0f;
			}
		}

		table->Push(olive::NodeValue::kSamples, QVariant::fromValue(buffer),
					this);
	}
};

olive::RenderTicketPtr MakeVideoTicket(olive::Node *node)
{
	olive::RenderTicketPtr ticket = std::make_shared<olive::RenderTicket>();
	ticket->setProperty("node", olive::QtUtils::PtrToValue(node));
	ticket->setProperty("time", QVariant::fromValue(olive::rational(0)));
	ticket->setProperty(
		"type", QVariant::fromValue(olive::RenderManager::kTypeVideo));
	ticket->setProperty(
		"vparam",
		QVariant::fromValue(olive::VideoParams(64, 64, olive::rational(1, 30),
											   olive::core::PixelFormat::U8,
											   4)));
	ticket->setProperty("aparam",
						QVariant::fromValue(olive::core::AudioParams()));
	ticket->setProperty("mode", int(olive::RenderMode::kOnline));
	return ticket;
}

olive::RenderTicketPtr MakeAudioTicket(olive::Node *node, bool waveforms,
									   bool clamp)
{
	olive::RenderTicketPtr ticket = std::make_shared<olive::RenderTicket>();
	ticket->setProperty("node", olive::QtUtils::PtrToValue(node));
	ticket->setProperty(
		"time",
		QVariant::fromValue(olive::TimeRange(olive::rational(0),
											 olive::rational(1))));
	ticket->setProperty(
		"type", QVariant::fromValue(olive::RenderManager::kTypeAudio));
	ticket->setProperty("enablewaveforms", waveforms);
	ticket->setProperty("clamp", clamp);
	ticket->setProperty(
		"aparam",
		QVariant::fromValue(olive::core::AudioParams(
			48000, olive::core::kChannelLayoutStereo,
			olive::core::SampleFormat::F32P)));
	ticket->setProperty(
		"vparam",
		QVariant::fromValue(olive::VideoParams(64, 64, olive::rational(1, 30),
											   olive::core::PixelFormat::U8,
											   4)));
	ticket->setProperty("mode", int(olive::RenderMode::kOnline));
	return ticket;
}

} // namespace

// ============================================================================
// RenderProcessor (null renderer = dry run path)
// ============================================================================

TEST(RenderProcessor, AudioTicketRendersAndClampsSamples)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = new ConstantSampleNode();
	node->setParent(&project);

	olive::RenderTicketPtr ticket = MakeAudioTicket(node, false, true);
	ticket->Start();

	olive::RenderProcessor::Process(ticket, nullptr, nullptr, nullptr);

	ASSERT_TRUE(ticket->HasResult());
	olive::core::SampleBuffer samples =
		ticket->Get().value<olive::core::SampleBuffer>();
	ASSERT_TRUE(samples.is_allocated());
	EXPECT_EQ(samples.channel_count(), 2);
	EXPECT_EQ(samples.sample_count(), size_t(48000));

	// The node emitted +2.0 everywhere; the ticket requested clamping.
	for (int ch = 0; ch < samples.channel_count(); ch++) {
		const float *data = samples.data(ch);
		for (size_t i = 0; i < samples.sample_count(); i++) {
			EXPECT_FLOAT_EQ(data[i], 1.0f);
		}
	}
}

TEST(RenderProcessor, AudioTicketWithoutClampKeepsSamples)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = new ConstantSampleNode();
	node->setParent(&project);

	olive::RenderTicketPtr ticket = MakeAudioTicket(node, false, false);
	ticket->Start();

	olive::RenderProcessor::Process(ticket, nullptr, nullptr, nullptr);

	ASSERT_TRUE(ticket->HasResult());
	olive::core::SampleBuffer samples =
		ticket->Get().value<olive::core::SampleBuffer>();
	ASSERT_TRUE(samples.is_allocated());
	ASSERT_GT(samples.sample_count(), size_t(0));

	const float *data = samples.data(0);
	for (size_t i = 0; i < samples.sample_count(); i++) {
		EXPECT_FLOAT_EQ(data[i], 2.0f);
	}
}

TEST(RenderProcessor, AudioTicketGeneratesWaveformWhenRequested)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = new ConstantSampleNode();
	node->setParent(&project);

	olive::RenderTicketPtr ticket = MakeAudioTicket(node, true, true);
	ticket->Start();

	olive::RenderProcessor::Process(ticket, nullptr, nullptr, nullptr);

	ASSERT_TRUE(ticket->HasResult());

	const QVariant waveform_var = ticket->property("waveform");
	ASSERT_TRUE(waveform_var.isValid());
	const olive::AudioVisualWaveform waveform =
		waveform_var.value<olive::AudioVisualWaveform>();
	EXPECT_EQ(waveform.channel_count(), 2);
}

TEST(RenderProcessor, AudioTicketWithoutNodeReturnsEmptyBuffer)
{
	olive::RenderTicketPtr ticket = MakeAudioTicket(nullptr, true, true);
	ticket->Start();

	olive::RenderProcessor::Process(ticket, nullptr, nullptr, nullptr);

	// With no node to traverse the processor still finishes with a (null)
	// SampleBuffer, and skips both clamping and waveform generation.
	ASSERT_TRUE(ticket->HasResult());
	const olive::core::SampleBuffer samples =
		ticket->Get().value<olive::core::SampleBuffer>();
	EXPECT_FALSE(samples.is_allocated());
	EXPECT_FALSE(ticket->property("waveform").isValid());
}

TEST(RenderProcessor, VideoTicketWithoutRendererFinishesWithoutResult)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);

	olive::RenderTicketPtr ticket = MakeVideoTicket(solid);
	ticket->Start();

	// A null render context is the "dry run": the graph is traversed (Solid
	// emits a shader job which is skipped) and the ticket finishes empty.
	olive::RenderProcessor::Process(ticket, nullptr, nullptr, nullptr);

	EXPECT_FALSE(ticket->IsRunning());
	EXPECT_EQ(ticket->GetFinishCount(), 1);
	EXPECT_FALSE(ticket->HasResult());
}

TEST(RenderProcessor, VideoTicketWithoutNodeFinishesWithoutResult)
{
	olive::RenderTicketPtr ticket = MakeVideoTicket(nullptr);
	ticket->Start();

	olive::RenderProcessor::Process(ticket, nullptr, nullptr, nullptr);

	EXPECT_FALSE(ticket->IsRunning());
	EXPECT_EQ(ticket->GetFinishCount(), 1);
	EXPECT_FALSE(ticket->HasResult());
}

TEST(RenderProcessor, CancelledTicketFinishesImmediately)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);

	olive::RenderTicketPtr ticket = MakeVideoTicket(solid);
	ticket->Start();
	ticket->Cancel();

	olive::RenderProcessor::Process(ticket, nullptr, nullptr, nullptr);

	EXPECT_EQ(ticket->GetFinishCount(), 1);
	EXPECT_FALSE(ticket->HasResult());
}

// ============================================================================
// RenderManager parameter plumbing (no instance required)
// ============================================================================

TEST(RenderManagerParams, RenderVideoParamsDefaults)
{
	const olive::VideoParams vparams(1920, 1080, olive::core::PixelFormat::U8,
									 4);
	const olive::core::AudioParams aparams;

	olive::RenderManager::RenderVideoParams params(nullptr, vparams, aparams,
												   olive::rational(5), nullptr,
												   olive::RenderMode::kOnline);

	EXPECT_EQ(params.node, nullptr);
	EXPECT_EQ(params.video_params, vparams);
	EXPECT_EQ(params.audio_params, aparams);
	EXPECT_EQ(params.time, olive::rational(5));
	EXPECT_EQ(params.color_manager, nullptr);
	EXPECT_EQ(params.mode, olive::RenderMode::kOnline);

	EXPECT_FALSE(params.use_cache);
	EXPECT_EQ(params.return_type, olive::RenderManager::kFrame);
	EXPECT_EQ(params.multicam, nullptr);

	EXPECT_TRUE(params.cache_dir.isEmpty());
	EXPECT_TRUE(params.cache_id.isEmpty());

	EXPECT_EQ(params.force_size, QSize(0, 0));
	EXPECT_EQ(params.force_channel_count, 0);
	EXPECT_TRUE(params.force_matrix.isIdentity());
	EXPECT_EQ(int(params.force_format),
			  int(olive::core::PixelFormat::INVALID));
	EXPECT_TRUE(params.force_color_output == nullptr);
	EXPECT_FALSE(params.force_color_transform.is_display());
	EXPECT_TRUE(params.force_color_transform.output().isEmpty());
}

TEST(RenderManagerParams, RenderAudioParamsDefaults)
{
	const olive::core::AudioParams aparams(
		48000, olive::core::kChannelLayoutStereo,
		olive::core::SampleFormat::F32P);
	const olive::TimeRange range(olive::rational(2), olive::rational(7));

	olive::RenderManager::RenderAudioParams params(nullptr, range, aparams,
												   olive::RenderMode::kOffline);

	EXPECT_EQ(params.node, nullptr);
	EXPECT_EQ(params.range, range);
	EXPECT_EQ(params.audio_params, aparams);
	EXPECT_FALSE(params.generate_waveforms);
	EXPECT_TRUE(params.clamp);
	EXPECT_EQ(params.mode, olive::RenderMode::kOffline);
}

TEST(RenderManagerParams, DryRunIntervalIsTenSeconds)
{
	EXPECT_EQ(olive::rational(olive::RenderManager::kDryRunInterval),
			  olive::rational(10));
}

// ============================================================================
// RenderJobTracker
// ============================================================================

TEST(RenderJobTracker, EmptyTrackerIsNeverCurrent)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime job;

	EXPECT_FALSE(tracker.isCurrent(olive::rational(0), job));
	EXPECT_TRUE(
		tracker.getCurrentSubRanges(olive::TimeRange(0, 10), job).isEmpty());
}

TEST(RenderJobTracker, InsertedRangeIsCurrentForSameAndNewerJobs)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime older;
	const olive::JobTime newer;

	tracker.insert(olive::TimeRange(0, 10), older);

	// A range rendered at job time T satisfies queries at T and later.
	EXPECT_TRUE(tracker.isCurrent(olive::rational(5), older));
	EXPECT_TRUE(tracker.isCurrent(olive::rational(5), newer));
}

TEST(RenderJobTracker, IsCurrentRespectsRangeBoundaries)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime job;

	tracker.insert(olive::TimeRange(0, 10), job);

	EXPECT_TRUE(tracker.isCurrent(olive::rational(0), job));
	EXPECT_FALSE(tracker.isCurrent(olive::rational(-1), job));
	// The out point is exclusive.
	EXPECT_FALSE(tracker.isCurrent(olive::rational(10), job));
	EXPECT_FALSE(tracker.isCurrent(olive::rational(11), job));
}

TEST(RenderJobTracker, ReinsertingSameRangeBumpsJobTime)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime older;
	const olive::JobTime newer;

	tracker.insert(olive::TimeRange(0, 10), older);
	EXPECT_TRUE(tracker.isCurrent(olive::rational(5), older));

	tracker.insert(olive::TimeRange(0, 10), newer);

	// The older job no longer describes the cached content.
	EXPECT_FALSE(tracker.isCurrent(olive::rational(5), older));
	EXPECT_TRUE(tracker.isCurrent(olive::rational(5), newer));
}

TEST(RenderJobTracker, InsertSplitsExistingRange)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime older;
	const olive::JobTime newer;

	tracker.insert(olive::TimeRange(0, 10), older);
	tracker.insert(olive::TimeRange(4, 6), newer);

	// The original range is split around the new one, keeping its job time.
	EXPECT_TRUE(tracker.isCurrent(olive::rational(2), older));
	EXPECT_TRUE(tracker.isCurrent(olive::rational(8), older));
	EXPECT_FALSE(tracker.isCurrent(olive::rational(5), older));
	EXPECT_TRUE(tracker.isCurrent(olive::rational(5), newer));
}

TEST(RenderJobTracker, InsertTrimsOverlappingRangeEnds)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime older;
	const olive::JobTime newer;

	tracker.insert(olive::TimeRange(0, 10), older);
	tracker.insert(olive::TimeRange(5, 15), newer);

	EXPECT_TRUE(tracker.isCurrent(olive::rational(2), older));
	EXPECT_FALSE(tracker.isCurrent(olive::rational(7), older));
	EXPECT_TRUE(tracker.isCurrent(olive::rational(7), newer));
	EXPECT_TRUE(tracker.isCurrent(olive::rational(12), newer));
	EXPECT_FALSE(tracker.isCurrent(olive::rational(16), newer));
}

TEST(RenderJobTracker, InsertRangeListTagsAllRanges)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime job;

	olive::TimeRangeList ranges;
	ranges.insert(olive::TimeRange(0, 5));
	ranges.insert(olive::TimeRange(10, 15));
	tracker.insert(ranges, job);

	EXPECT_TRUE(tracker.isCurrent(olive::rational(2), job));
	EXPECT_TRUE(tracker.isCurrent(olive::rational(12), job));
	EXPECT_FALSE(tracker.isCurrent(olive::rational(7), job));
}

TEST(RenderJobTracker, GetCurrentSubRangesClipsToQueryRange)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime job;

	tracker.insert(olive::TimeRange(0, 10), job);

	const olive::TimeRangeList sub =
		tracker.getCurrentSubRanges(olive::TimeRange(4, 20), job);
	ASSERT_EQ(sub.size(), 1);
	EXPECT_EQ(*sub.begin(), olive::TimeRange(4, 10));
}

TEST(RenderJobTracker, GetCurrentSubRangesIgnoresNewerJobs)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime older;
	const olive::JobTime newer;

	tracker.insert(olive::TimeRange(0, 10), newer);

	// Querying with an older job time sees nothing current.
	EXPECT_TRUE(
		tracker.getCurrentSubRanges(olive::TimeRange(0, 10), older).isEmpty());
}

TEST(RenderJobTracker, GetCurrentSubRangesMergesAdjacentJobs)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime older;
	const olive::JobTime newer;

	tracker.insert(olive::TimeRange(0, 5), older);
	tracker.insert(olive::TimeRange(5, 10), newer);

	// Both jobs are current for `newer`; touching ranges merge into one.
	const olive::TimeRangeList sub =
		tracker.getCurrentSubRanges(olive::TimeRange(0, 10), newer);
	ASSERT_EQ(sub.size(), 1);
	EXPECT_EQ(*sub.begin(), olive::TimeRange(0, 10));
}

TEST(RenderJobTracker, ClearDropsAllJobs)
{
	olive::RenderJobTracker tracker;
	const olive::JobTime job;

	tracker.insert(olive::TimeRange(0, 10), job);
	EXPECT_TRUE(tracker.isCurrent(olive::rational(5), job));

	tracker.clear();
	EXPECT_FALSE(tracker.isCurrent(olive::rational(5), job));
}

// ============================================================================
// SubtitleParams
// ============================================================================

TEST(SubtitleParams, DefaultsAreEmptyEnabledStreamZero)
{
	const olive::SubtitleParams params;

	EXPECT_FALSE(params.is_valid());
	EXPECT_EQ(params.duration(), olive::rational(0));
	EXPECT_EQ(params.stream_index(), 0);
	EXPECT_TRUE(params.enabled());
}

TEST(SubtitleParams, DurationFollowsLastSubtitle)
{
	olive::SubtitleParams params;
	params.push_back(
		olive::Subtitle(olive::TimeRange(0, 2), QStringLiteral("one")));
	params.push_back(
		olive::Subtitle(olive::TimeRange(3, 5), QStringLiteral("two")));

	EXPECT_TRUE(params.is_valid());
	EXPECT_EQ(params.duration(), olive::rational(5));

	params.set_stream_index(3);
	params.set_enabled(false);
	EXPECT_EQ(params.stream_index(), 3);
	EXPECT_FALSE(params.enabled());
}

TEST(SubtitleParams, SubtitleAccessorsRoundTrip)
{
	olive::Subtitle sub(olive::TimeRange(1, 4), QStringLiteral("hello"));
	EXPECT_EQ(sub.time(), olive::TimeRange(1, 4));
	EXPECT_EQ(sub.text(), QStringLiteral("hello"));

	sub.set_time(olive::TimeRange(2, 6));
	sub.set_text(QStringLiteral("world"));
	EXPECT_EQ(sub.time(), olive::TimeRange(2, 6));
	EXPECT_EQ(sub.text(), QStringLiteral("world"));

	const olive::Subtitle def;
	EXPECT_TRUE(def.text().isEmpty());
}

TEST(SubtitleParams, GenerateAssHeaderContainsRequiredSections)
{
	const QString header = olive::SubtitleParams::GenerateASSHeader();

	EXPECT_TRUE(header.contains(QStringLiteral("[Script Info]\r\n")));
	EXPECT_TRUE(header.contains(QStringLiteral("ScriptType: v4.00+\r\n")));
	EXPECT_TRUE(header.contains(QStringLiteral("PlayResX: 384\r\n")));
	EXPECT_TRUE(header.contains(QStringLiteral("PlayResY: 288\r\n")));
	EXPECT_TRUE(
		header.contains(QStringLiteral("ScaledBorderAndShadow: yes\r\n")));
	EXPECT_TRUE(header.contains(QStringLiteral("[V4+ Styles]\r\n")));
	EXPECT_TRUE(header.contains(QStringLiteral("Style: Default,Arial,16,")));
	EXPECT_TRUE(
		header.contains(QStringLiteral("&Hffffff,&Hffffff,&H0,&H0,")));
	EXPECT_TRUE(header.contains(QStringLiteral("[Events]\r\n")));
	EXPECT_TRUE(header.contains(
		QStringLiteral("Format: Layer, Start, End, Style, Name, MarginL, "
					   "MarginR, MarginV, Effect, Text")));
	EXPECT_TRUE(header.endsWith(QStringLiteral("\r\n")));
}

TEST(SubtitleParams, SaveLoadRoundTrip)
{
	olive::SubtitleParams params;
	params.set_stream_index(2);
	params.set_enabled(false);
	params.push_back(olive::Subtitle(
		olive::TimeRange(olive::rational(0), olive::rational(1, 2)),
		QStringLiteral("Hello, world!")));
	params.push_back(olive::Subtitle(
		olive::TimeRange(olive::rational(3, 4), olive::rational(2)),
		QStringLiteral("Second <line> & more")));

	QString xml;
	{
		QXmlStreamWriter writer(&xml);
		writer.writeStartElement(QStringLiteral("root"));
		params.Save(&writer);
		writer.writeEndElement();
	}

	olive::SubtitleParams loaded;
	// Pre-existing content must be cleared by Load.
	loaded.push_back(
		olive::Subtitle(olive::TimeRange(9, 10), QStringLiteral("junk")));

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement()); // position on <root>
	loaded.Load(&reader);

	EXPECT_EQ(loaded.stream_index(), 2);
	EXPECT_FALSE(loaded.enabled());
	ASSERT_EQ(loaded.size(), size_t(2));
	EXPECT_EQ(loaded.at(0).time().in(), olive::rational(0));
	EXPECT_EQ(loaded.at(0).time().out(), olive::rational(1, 2));
	EXPECT_EQ(loaded.at(0).text(), QStringLiteral("Hello, world!"));
	EXPECT_EQ(loaded.at(1).time().in(), olive::rational(3, 4));
	EXPECT_EQ(loaded.at(1).time().out(), olive::rational(2));
	EXPECT_EQ(loaded.at(1).text(), QStringLiteral("Second <line> & more"));
}

// ============================================================================
// ManagedColor
// ============================================================================

TEST(ManagedColor, DefaultConstructionHasNoTransforms)
{
	const olive::ManagedColor color;

	EXPECT_FLOAT_EQ(color.red(), 0.0f);
	EXPECT_FLOAT_EQ(color.green(), 0.0f);
	EXPECT_FLOAT_EQ(color.blue(), 0.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 0.0f);
	EXPECT_TRUE(color.color_input().isEmpty());
	EXPECT_FALSE(color.color_output().is_display());
	EXPECT_TRUE(color.color_output().output().isEmpty());
}

TEST(ManagedColor, RgbaConstructorPreservesChannels)
{
	const olive::ManagedColor color(0.25, 0.5, 0.75, 0.5);

	EXPECT_FLOAT_EQ(color.red(), 0.25f);
	EXPECT_FLOAT_EQ(color.green(), 0.5f);
	EXPECT_FLOAT_EQ(color.blue(), 0.75f);
	EXPECT_FLOAT_EQ(color.alpha(), 0.5f);
}

TEST(ManagedColor, ColorCopyConstructorPreservesChannels)
{
	const olive::core::Color base(0.1f, 0.2f, 0.3f, 1.0f);
	const olive::ManagedColor color(base);

	EXPECT_FLOAT_EQ(color.red(), base.red());
	EXPECT_FLOAT_EQ(color.green(), base.green());
	EXPECT_FLOAT_EQ(color.blue(), base.blue());
	EXPECT_FLOAT_EQ(color.alpha(), base.alpha());
}

TEST(ManagedColor, RawDataConstructorDecodesU8)
{
	const char data[4] = { char(255), char(128), char(0), char(64) };
	const olive::ManagedColor color(data, olive::core::PixelFormat::U8, 4);

	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_NEAR(color.green(), 128.0 / 255.0, 1e-6);
	EXPECT_FLOAT_EQ(color.blue(), 0.0f);
	EXPECT_NEAR(color.alpha(), 64.0 / 255.0, 1e-6);
}

TEST(ManagedColor, ColorInputAndOutputRoundTrip)
{
	olive::ManagedColor color;

	color.set_color_input(QStringLiteral("linear"));
	EXPECT_EQ(color.color_input(), QStringLiteral("linear"));

	color.set_color_output(olive::ColorTransform(QStringLiteral("sRGB")));
	EXPECT_FALSE(color.color_output().is_display());
	EXPECT_EQ(color.color_output().output(), QStringLiteral("sRGB"));

	color.set_color_output(olive::ColorTransform(QStringLiteral("sRGB"),
												 QStringLiteral("Filmic"),
												 QStringLiteral("None")));
	EXPECT_TRUE(color.color_output().is_display());
	EXPECT_EQ(color.color_output().display(), QStringLiteral("sRGB"));
	EXPECT_EQ(color.color_output().view(), QStringLiteral("Filmic"));
	EXPECT_EQ(color.color_output().look(), QStringLiteral("None"));
}

// ============================================================================
// Texture (dummy, no renderer)
// ============================================================================

TEST(RenderTexture, DummyTextureExposesParams)
{
	const olive::VideoParams params(320, 240, olive::core::PixelFormat::U8, 4);
	olive::Texture texture(params);

	EXPECT_TRUE(texture.IsDummy());
	EXPECT_EQ(texture.renderer(), nullptr);
	EXPECT_EQ(texture.params(), params);
	EXPECT_EQ(texture.width(), 320);
	EXPECT_EQ(texture.height(), 240);
	EXPECT_EQ(texture.channel_count(), 4);
	EXPECT_EQ(texture.divider(), 1);
	EXPECT_EQ(texture.pixel_aspect_ratio(), olive::rational(1));
	EXPECT_EQ(texture.virtual_resolution(), QVector2D(320, 240));
	EXPECT_EQ(int(texture.format()), int(olive::core::PixelFormat::U8));
	EXPECT_FALSE(texture.id().isValid());
	EXPECT_FALSE(texture.IsJob());
	EXPECT_EQ(texture.job(), nullptr);
	EXPECT_TRUE(texture.frame() == nullptr);
}

TEST(RenderTexture, DummyTextureHonorsDivider)
{
	const olive::VideoParams params(320, 240, olive::core::PixelFormat::U8, 4,
									olive::rational(1),
									olive::VideoParams::kInterlaceNone, 2);
	const olive::Texture texture(params);

	EXPECT_EQ(texture.divider(), 2);
	EXPECT_EQ(texture.width(), 160);
	EXPECT_EQ(texture.height(), 120);
}

TEST(RenderTexture, JobTextureCarriesJobAndParams)
{
	const olive::VideoParams params(64, 64, olive::core::PixelFormat::F32, 4);

	olive::AcceleratedJob job;
	job.Insert(QStringLiteral("value_in"),
			   olive::NodeValue(olive::NodeValue::kFloat, 2.5));

	const olive::TexturePtr texture = olive::Texture::Job(params, job);
	ASSERT_TRUE(texture != nullptr);
	EXPECT_TRUE(texture->IsDummy());
	EXPECT_TRUE(texture->IsJob());
	ASSERT_TRUE(texture->job() != nullptr);
	EXPECT_TRUE(
		texture->job()->GetValues().contains(QStringLiteral("value_in")));
	EXPECT_EQ(texture->job()->Get(QStringLiteral("value_in")).toDouble(), 2.5);
	EXPECT_EQ(texture->params(), params);
}

TEST(RenderTexture, ToJobCreatesJobTextureWithSameParams)
{
	const olive::VideoParams params(128, 72, olive::core::PixelFormat::U8, 4);
	olive::Texture dummy(params);

	const olive::AcceleratedJob job;
	const olive::TexturePtr job_tex = dummy.toJob(job);

	ASSERT_TRUE(job_tex != nullptr);
	EXPECT_FALSE(dummy.IsJob());
	EXPECT_TRUE(job_tex->IsJob());
	EXPECT_EQ(job_tex->params(), dummy.params());
}

TEST(RenderTexture, UploadDownloadOnDummyAreNoOps)
{
	const olive::VideoParams params(16, 16, olive::core::PixelFormat::U8, 4);
	olive::Texture texture(params);

	// With no renderer backend both calls must return without touching data.
	uint8_t buffer[16 * 16 * 4];
	memset(buffer, 0xAB, sizeof(buffer));
	texture.Upload(buffer, 16 * 4);
	texture.Download(buffer, 16 * 4);
	EXPECT_EQ(buffer[0], uint8_t(0xAB));
}

TEST(RenderTexture, DefaultInterpolationIsMipmappedLinear)
{
	EXPECT_EQ(int(olive::Texture::kDefaultInterpolation),
			  int(olive::Texture::kMipmappedLinear));
}
