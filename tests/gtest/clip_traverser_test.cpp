#include <gtest/gtest.h>

#include <climits>
#include <limits>

#include <QTransform>
#include <QVector>

#include "node/block/clip/clip.h"
#include "node/generator/solid/solid.h"
#include "node/globals.h"
#include "node/input/time/timeinput.h"
#include "node/output/track/track.h"
#include "node/output/viewer/viewer.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "node/traverser.h"
#include "olive/core/util/rational.h"
#include "olive/core/util/timerange.h"
#include "render/framehashcache.h"
#include "render/loopmode.h"
#include "render/playbackcache.h"

namespace
{

// Records the loop mode seen in NodeGlobals so the traverser's clip loop
// mode pick-up can be observed.
class LoopModeProbeNode : public olive::Node {
public:
	LoopModeProbeNode() = default;

	NODE_DEFAULT_FUNCTIONS(LoopModeProbeNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Loop Mode Probe");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.loopmodeprobe");
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

		last_loop_mode_ = globals.loop_mode();
		table->Push(olive::NodeValue::kFloat, 0.0, this);
	}

	olive::LoopMode last_loop_mode() const
	{
		return last_loop_mode_;
	}

private:
	mutable olive::LoopMode last_loop_mode_ = olive::LoopMode::kLoopModeOff;
};

// Node with two connectable inputs and a configurable gizmo transformation,
// used to verify which nodes the traverser accumulates transforms from.
class GizmoProbeNode : public olive::Node {
public:
	GizmoProbeNode()
	{
		AddInput(QStringLiteral("a_in"), olive::NodeValue::kFloat, 0.0);
		AddInput(QStringLiteral("b_in"), olive::NodeValue::kFloat, 0.0);
	}

	NODE_DEFAULT_FUNCTIONS(GizmoProbeNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Gizmo Probe");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.gizmoprobe");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return {};
	}

	void SetGizmoTransform(const QTransform &t)
	{
		t_ = t;
	}

	virtual QTransform
	GizmoTransformation(const olive::NodeValueRow &row,
						const olive::NodeGlobals &globals) const override
	{
		Q_UNUSED(row)
		Q_UNUSED(globals)

		return t_;
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)
		Q_UNUSED(globals)

		table->Push(olive::NodeValue::kFloat, 0.0, this);
	}

	static QString InputA()
	{
		return QStringLiteral("a_in");
	}

	static QString InputB()
	{
		return QStringLiteral("b_in");
	}

private:
	QTransform t_;
};

olive::ClipBlock *CreateClip(olive::Project *project,
							 const olive::core::rational &length)
{
	auto *clip = new olive::ClipBlock();
	clip->setParent(project);
	clip->set_length_and_media_out(length);
	return clip;
}

olive::SolidGenerator *CreateSolid(olive::Project *project)
{
	auto *solid = new olive::SolidGenerator();
	solid->setParent(project);
	return solid;
}

// Generates the clip's output table at a single time with a fresh traverser
// (the traverser caches tables per node+range, so reusing one would return
// stale values after the clip's parameters change).
olive::NodeValueTable GenerateClipTable(const olive::ClipBlock *clip,
										const olive::core::rational &time)
{
	olive::NodeTraverser traverser;
	return traverser.GenerateTable(
		clip, olive::TimeRange(time, time + olive::core::rational(1, 30)));
}

double GenerateClipTimeValue(const olive::ClipBlock *clip,
							 const olive::core::rational &time)
{
	olive::NodeValueTable table = GenerateClipTable(clip, time);
	olive::NodeValue v = table.Get(olive::NodeValue::kFloat);
	if (v.type() != olive::NodeValue::kFloat) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	return v.toDouble();
}

} // namespace

TEST(ClipBlock, DefaultState)
{
	olive::ClipBlock clip;

	EXPECT_DOUBLE_EQ(clip.speed(), 1.0);
	EXPECT_FALSE(clip.reverse());
	EXPECT_EQ(clip.loop_mode(), olive::LoopMode::kLoopModeOff);
	EXPECT_EQ(clip.media_in(), olive::core::rational(0));
	EXPECT_FALSE(clip.maintain_audio_pitch());
	EXPECT_FALSE(clip.IsAutocaching());
	EXPECT_EQ(clip.in_transition(), nullptr);
	EXPECT_EQ(clip.out_transition(), nullptr);
	EXPECT_EQ(clip.connected_viewer(), nullptr);
	EXPECT_EQ(clip.GetTrackType(), olive::Track::kNone);
	EXPECT_TRUE(clip.block_links().isEmpty());

	EXPECT_EQ(clip.length(), olive::core::rational(0));
	EXPECT_EQ(clip.GetVideoCacheRange(),
			  olive::TimeRange(olive::core::rational(0), olive::core::rational(0)));
	EXPECT_EQ(clip.GetAudioCacheRange(),
			  olive::TimeRange(olive::core::rational(0), olive::core::rational(0)));

	EXPECT_EQ(clip.id(), QStringLiteral("org.olivevideoeditor.Olive.clip"));
	EXPECT_EQ(clip.Name(), QStringLiteral("Clip"));
	EXPECT_FALSE(clip.Description().isEmpty());
	EXPECT_TRUE(clip.Category().contains(olive::Node::kCategoryTimeline));
}

TEST(ClipBlock, SpeedReverseLoopPitchAutocacheAccessors)
{
	olive::ClipBlock clip;

	clip.SetStandardValue(olive::ClipBlock::kSpeedInput, 2.5);
	EXPECT_DOUBLE_EQ(clip.speed(), 2.5);

	clip.set_reverse(true);
	EXPECT_TRUE(clip.reverse());
	clip.set_reverse(false);
	EXPECT_FALSE(clip.reverse());

	clip.set_loop_mode(olive::LoopMode::kLoopModeLoop);
	EXPECT_EQ(clip.loop_mode(), olive::LoopMode::kLoopModeLoop);
	clip.set_loop_mode(olive::LoopMode::kLoopModeClamp);
	EXPECT_EQ(clip.loop_mode(), olive::LoopMode::kLoopModeClamp);
	clip.set_loop_mode(olive::LoopMode::kLoopModeOff);
	EXPECT_EQ(clip.loop_mode(), olive::LoopMode::kLoopModeOff);

	clip.set_maintain_audio_pitch(true);
	EXPECT_TRUE(clip.maintain_audio_pitch());

	clip.SetAutocache(true);
	EXPECT_TRUE(clip.IsAutocaching());
	clip.SetAutocache(false);
	EXPECT_FALSE(clip.IsAutocaching());
}

TEST(ClipBlock, LoopModeChangeEmitsPreviewChanged)
{
	olive::ClipBlock clip;

	int emissions = 0;
	QObject::connect(&clip, &olive::Block::PreviewChanged,
					 [&emissions]() { ++emissions; });

	clip.set_loop_mode(olive::LoopMode::kLoopModeLoop);
	EXPECT_EQ(emissions, 1);

	clip.set_loop_mode(olive::LoopMode::kLoopModeClamp);
	EXPECT_EQ(emissions, 2);
}

TEST(ClipBlock, MediaInAccessor)
{
	olive::ClipBlock clip;

	clip.set_media_in(olive::core::rational(5));
	EXPECT_EQ(clip.media_in(), olive::core::rational(5));
	EXPECT_EQ(clip.GetStandardValue(olive::ClipBlock::kMediaInInput)
				  .value<olive::core::rational>(),
			  olive::core::rational(5));
}

TEST(ClipBlock, InputTimeAdjustmentPassesThroughByDefault)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	const olive::TimeRange range(olive::core::rational(2),
								 olive::core::rational(4));

	// A default clip (speed 1, no reverse, no media in) maps times unchanged
	EXPECT_EQ(clip->InputTimeAdjustment(olive::ClipBlock::kBufferIn, -1, range,
										true),
			  range);

	// Non-buffer inputs never adjust time
	EXPECT_EQ(clip->InputTimeAdjustment(olive::ClipBlock::kSpeedInput, -1,
										range, true),
			  range);
	EXPECT_EQ(clip->OutputTimeAdjustment(olive::ClipBlock::kSpeedInput, -1,
										 range),
			  range);
}

TEST(ClipBlock, InputTimeAdjustmentAppliesSpeed)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	EXPECT_EQ(clip->InputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(4),
							   olive::core::rational(8)));

	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 0.5);
	EXPECT_EQ(clip->InputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(1),
							   olive::core::rational(2)));

	// Media in is added after the speed multiplication
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	clip->set_media_in(olive::core::rational(3));
	EXPECT_EQ(clip->InputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(7),
							   olive::core::rational(11)));
}

TEST(ClipBlock, InputTimeAdjustmentZeroSpeedHoldsAtMediaIn)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 0.0);
	clip->set_media_in(olive::core::rational(5));

	// Zero speed collapses every sequence time onto the media in point
	const olive::TimeRange adjusted = clip->InputTimeAdjustment(
		olive::ClipBlock::kBufferIn, -1,
		olive::TimeRange(olive::core::rational(2), olive::core::rational(4)),
		true);
	EXPECT_EQ(adjusted.in(), olive::core::rational(5));
	EXPECT_EQ(adjusted.out(), olive::core::rational(5));
}

TEST(ClipBlock, InputTimeAdjustmentAppliesReverse)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	clip->set_reverse(true);

	// Reverse mirrors time around the clip length; TimeRange normalizes the
	// resulting inverted range
	EXPECT_EQ(clip->InputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(3)),
				  true),
			  olive::TimeRange(olive::core::rational(7),
							   olive::core::rational(8)));
}

TEST(ClipBlock, InputTimeAdjustmentCombinesReverseSpeedAndMediaIn)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	clip->set_reverse(true);
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	clip->set_media_in(olive::core::rational(5));

	// (10 - 2) * 2 + 5 = 21, (10 - 3) * 2 + 5 = 19, normalized to [19, 21]
	EXPECT_EQ(clip->InputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(3)),
				  true),
			  olive::TimeRange(olive::core::rational(19),
							   olive::core::rational(21)));
}

TEST(ClipBlock, InputTimeAdjustmentPassesThroughInfinities)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	clip->set_reverse(true);
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	clip->set_media_in(olive::core::rational(5));

	const olive::core::rational kMin(INT_MIN);
	const olive::core::rational kMax(INT_MAX);
	const olive::TimeRange infinite(kMin, kMax);

	EXPECT_EQ(clip->InputTimeAdjustment(olive::ClipBlock::kBufferIn, -1,
										infinite, true),
			  infinite);
	EXPECT_EQ(clip->OutputTimeAdjustment(olive::ClipBlock::kBufferIn, -1,
										 infinite),
			  infinite);
}

TEST(ClipBlock, OutputTimeAdjustmentInvertsInputAdjustment)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	clip->set_media_in(olive::core::rational(5));

	// Media time is converted back by subtracting media in and dividing speed
	EXPECT_EQ(clip->OutputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  olive::TimeRange(olive::core::rational(5),
								   olive::core::rational(9))),
			  olive::TimeRange(olive::core::rational(0),
							   olive::core::rational(2)));

	// Round trip through both adjustments returns the original range
	const olive::TimeRange range(olive::core::rational(1),
								 olive::core::rational(3));
	EXPECT_EQ(clip->OutputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  clip->InputTimeAdjustment(olive::ClipBlock::kBufferIn, -1,
											range, true)),
			  range);

	// Round trip also holds in reverse
	clip->set_reverse(true);
	EXPECT_EQ(clip->OutputTimeAdjustment(
				  olive::ClipBlock::kBufferIn, -1,
				  clip->InputTimeAdjustment(olive::ClipBlock::kBufferIn, -1,
											range, true)),
			  range);
}

TEST(ClipBlock, MediaRangeReflectsSpeedReverseAndMediaIn)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	EXPECT_EQ(clip->media_range(),
			  olive::TimeRange(olive::core::rational(0),
							   olive::core::rational(10)));

	clip->set_media_in(olive::core::rational(5));
	EXPECT_EQ(clip->media_range(),
			  olive::TimeRange(olive::core::rational(5),
							   olive::core::rational(15)));

	// A 2x clip consumes twice its length in media time
	clip->set_media_in(olive::core::rational(0));
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	EXPECT_EQ(clip->media_range(),
			  olive::TimeRange(olive::core::rational(0),
							   olive::core::rational(20)));

	// Reverse maps the same media extent (the range normalizes)
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 1.0);
	clip->set_media_in(olive::core::rational(5));
	clip->set_reverse(true);
	EXPECT_EQ(clip->media_range(),
			  olive::TimeRange(olive::core::rational(5),
							   olive::core::rational(15)));
}

TEST(ClipBlock, SetLengthAndMediaOutInReversePreservesMediaOut)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	clip->set_reverse(true);

	// Trimming the out point of a reversed clip moves the media in point so
	// that the media out point is preserved
	clip->set_length_and_media_out(olive::core::rational(4));

	EXPECT_EQ(clip->length(), olive::core::rational(4));
	EXPECT_EQ(clip->media_in(), olive::core::rational(6));
	EXPECT_EQ(clip->media_range(),
			  olive::TimeRange(olive::core::rational(6),
							   olive::core::rational(10)));
}

TEST(ClipBlock, SetLengthAndMediaInForwardAdjustsMediaIn)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	// Trimming the in point of a forward clip pushes the media in point
	// forward by the removed amount
	clip->set_length_and_media_in(olive::core::rational(4));

	EXPECT_EQ(clip->length(), olive::core::rational(4));
	EXPECT_EQ(clip->media_in(), olive::core::rational(6));
	EXPECT_EQ(clip->media_range(),
			  olive::TimeRange(olive::core::rational(6),
							   olive::core::rational(10)));
}

TEST(ClipBlock, ConnectedCacheAccessors)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	EXPECT_EQ(clip->connected_video_cache(), nullptr);
	EXPECT_EQ(clip->connected_audio_cache(), nullptr);
	EXPECT_EQ(clip->thumbnails(), nullptr);
	EXPECT_EQ(clip->waveform(), nullptr);

	olive::SolidGenerator *solid = CreateSolid(&project);
	olive::Node::ConnectEdge(solid,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	EXPECT_EQ(clip->connected_video_cache(), solid->video_frame_cache());
	EXPECT_EQ(clip->connected_audio_cache(), solid->audio_playback_cache());
	EXPECT_EQ(clip->thumbnails(), solid->thumbnail_cache());
	EXPECT_EQ(clip->waveform(), solid->waveform_cache());
}

TEST(ClipBlock, InvalidateCacheTransformsMediaTimeToSequenceTime)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);

	olive::SolidGenerator *solid = CreateSolid(&project);
	olive::Node::ConnectEdge(solid,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	QVector<olive::TimeRange> invalidated;
	QObject::connect(clip->video_frame_cache(),
					 &olive::PlaybackCache::Invalidated,
					 [&invalidated](const olive::TimeRange &r) {
						 invalidated.append(r);
					 });

	// An invalidation in media time [4,8] covers sequence time [2,4] at 2x
	solid->InvalidateCache(olive::TimeRange(olive::core::rational(4),
											olive::core::rational(8)),
						   olive::SolidGenerator::kColorInput);

	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(),
			  olive::TimeRange(olive::core::rational(2),
							   olive::core::rational(4)));
}

TEST(ClipBlock, InvalidateCacheReverseTransformsRange)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	clip->set_reverse(true);

	olive::SolidGenerator *solid = CreateSolid(&project);
	olive::Node::ConnectEdge(solid,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	QVector<olive::TimeRange> invalidated;
	QObject::connect(clip->video_frame_cache(),
					 &olive::PlaybackCache::Invalidated,
					 [&invalidated](const olive::TimeRange &r) {
						 invalidated.append(r);
					 });

	// Media time [4,8] maps to sequence time [2,6] when reversed
	solid->InvalidateCache(olive::TimeRange(olive::core::rational(4),
											olive::core::rational(8)),
						   olive::SolidGenerator::kColorInput);

	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(),
			  olive::TimeRange(olive::core::rational(2),
							   olive::core::rational(6)));
}

TEST(ClipBlock, InvalidateCacheZeroSpeedInvalidatesWholeClip)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 0.0);

	olive::SolidGenerator *solid = CreateSolid(&project);
	olive::Node::ConnectEdge(solid,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	QVector<olive::TimeRange> invalidated;
	QObject::connect(clip->video_frame_cache(),
					 &olive::PlaybackCache::Invalidated,
					 [&invalidated](const olive::TimeRange &r) {
						 invalidated.append(r);
					 });

	// With zero speed any media invalidation affects the whole clip
	solid->InvalidateCache(olive::TimeRange(olive::core::rational(4),
											olive::core::rational(8)),
						   olive::SolidGenerator::kColorInput);

	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(),
			  olive::TimeRange(olive::core::rational(0),
							   olive::core::rational(10)));
}

TEST(ClipBlock, InvalidateCacheWithVideoTrackReachesConnectedCaches)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *track = new olive::Track();
	track->setParent(&project);
	track->set_type(olive::Track::kVideo);

	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	track->AppendBlock(clip);

	olive::SolidGenerator *solid = CreateSolid(&project);
	olive::Node::ConnectEdge(solid,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	QVector<olive::TimeRange> invalidated;
	QObject::connect(solid->video_frame_cache(),
					 &olive::PlaybackCache::Invalidated,
					 [&invalidated](const olive::TimeRange &r) {
						 invalidated.append(r);
					 });
	QVector<olive::TimeRange> requested;
	QObject::connect(solid->video_frame_cache(),
					 &olive::PlaybackCache::Requested,
					 [&requested](olive::ViewerOutput *, const olive::TimeRange &r) {
						 requested.append(r);
					 });

	// Without autocache the connected cache is invalidated but not requested
	clip->InvalidateCache(olive::TimeRange(olive::core::rational(4),
										   olive::core::rational(8)),
						  olive::ClipBlock::kBufferIn, -1,
						  olive::Node::InvalidateCacheOptions());
	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(),
			  olive::TimeRange(olive::core::rational(4),
							   olive::core::rational(8)));
	EXPECT_TRUE(requested.isEmpty());

	// Enabling autocache re-requests everything currently invalidated (a
	// fresh cache has no validated ranges, so the full media range)
	clip->SetAutocache(true);
	ASSERT_GE(requested.size(), 1);
	EXPECT_EQ(requested.first(),
			  olive::TimeRange(olive::core::rational(0),
							   olive::core::rational(10)));

	// With autocache on, invalidations are also requested
	invalidated.clear();
	requested.clear();
	clip->InvalidateCache(olive::TimeRange(olive::core::rational(4),
										   olive::core::rational(8)),
						  olive::ClipBlock::kBufferIn, -1,
						  olive::Node::InvalidateCacheOptions());
	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(),
			  olive::TimeRange(olive::core::rational(4),
							   olive::core::rational(8)));
	ASSERT_EQ(requested.size(), 1);
	EXPECT_EQ(requested.first(),
			  olive::TimeRange(olive::core::rational(4),
							   olive::core::rational(8)));
}

TEST(ClipBlock, DiscardCacheInvalidatesConnectedNodeCache)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *track = new olive::Track();
	track->setParent(&project);
	track->set_type(olive::Track::kVideo);

	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	track->AppendBlock(clip);

	olive::SolidGenerator *solid = CreateSolid(&project);
	olive::Node::ConnectEdge(solid,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	QVector<olive::TimeRange> invalidated;
	QObject::connect(solid->video_frame_cache(),
					 &olive::PlaybackCache::Invalidated,
					 [&invalidated](const olive::TimeRange &r) {
						 invalidated.append(r);
					 });

	clip->DiscardCache();

	const olive::core::rational kMin(INT_MIN);
	const olive::core::rational kMax(INT_MAX);
	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(), olive::TimeRange(kMin, kMax));
}

TEST(ClipBlock, AddCachePassthroughFromUnvalidatedSourceAddsNoPassthroughs)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(10));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(10));

	// Passthroughs are only added for validated ranges of the source caches,
	// and a fresh cache has none
	b->AddCachePassthroughFrom(a);

	EXPECT_TRUE(b->video_frame_cache()->GetPassthroughs().empty());
	EXPECT_TRUE(b->audio_playback_cache()->GetPassthroughs().empty());
}

TEST(ClipBlock, GetValueHintForBufferWithoutTrackHasNoPreference)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	// With no track the clip cannot prefer texture or samples and falls back
	// to the default (typeless) hint
	EXPECT_TRUE(clip->GetValueHintForInput(olive::ClipBlock::kBufferIn)
					.types()
					.isEmpty());
}

TEST(ClipBlock, FindMulticamReturnsNullWithoutMulticam)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	EXPECT_EQ(clip->FindMulticam(), nullptr);

	olive::SolidGenerator *solid = CreateSolid(&project);
	olive::Node::ConnectEdge(solid,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));
	EXPECT_EQ(clip->FindMulticam(), nullptr);
}

TEST(ClipTraverser, GenerateTablePropagatesSpeedAdjustedTime)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);

	auto *time = new olive::TimeInput();
	time->setParent(&project);
	olive::Node::ConnectEdge(time,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	// The connected node is evaluated in media time: sequence time 3 at 2x
	// speed reaches the time node as 6
	EXPECT_DOUBLE_EQ(GenerateClipTimeValue(clip, olive::core::rational(3)),
					 6.0);
}

TEST(ClipTraverser, GenerateTablePropagatesMediaInOffset)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	clip->set_media_in(olive::core::rational(5));

	auto *time = new olive::TimeInput();
	time->setParent(&project);
	olive::Node::ConnectEdge(time,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	EXPECT_DOUBLE_EQ(GenerateClipTimeValue(clip, olive::core::rational(3)),
					 8.0);
}

TEST(ClipTraverser, GenerateTablePropagatesReverseTime)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	clip->set_reverse(true);

	auto *time = new olive::TimeInput();
	time->setParent(&project);
	olive::Node::ConnectEdge(time,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	// Reverse maps the frame range [t, t+1/30) onto media [7, 7-1/30), so the
	// connected node is evaluated at 7 - 1/30
	EXPECT_DOUBLE_EQ(GenerateClipTimeValue(clip, olive::core::rational(3)),
					 7.0 - 1.0 / 30.0);
}

TEST(ClipTraverser, GenerateTableCombinesReverseSpeedAndMediaIn)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));
	clip->set_reverse(true);
	clip->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	clip->set_media_in(olive::core::rational(5));

	auto *time = new olive::TimeInput();
	time->setParent(&project);
	olive::Node::ConnectEdge(time,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	// (10 - 1/30 - 3) * 2 + 5 = 19 - 2/30
	EXPECT_DOUBLE_EQ(GenerateClipTimeValue(clip, olive::core::rational(3)),
					 19.0 - 2.0 / 30.0);
}

TEST(ClipTraverser, GenerateTablePicksUpClipLoopMode)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::ClipBlock *clip = CreateClip(&project, olive::core::rational(10));

	auto *probe = new LoopModeProbeNode();
	probe->setParent(&project);
	olive::Node::ConnectEdge(probe,
							 olive::NodeInput(clip, olive::ClipBlock::kBufferIn));

	GenerateClipTable(clip, olive::core::rational(0));
	EXPECT_EQ(probe->last_loop_mode(), olive::LoopMode::kLoopModeOff);

	clip->set_loop_mode(olive::LoopMode::kLoopModeLoop);
	GenerateClipTable(clip, olive::core::rational(0));
	EXPECT_EQ(probe->last_loop_mode(), olive::LoopMode::kLoopModeLoop);

	clip->set_loop_mode(olive::LoopMode::kLoopModeClamp);
	GenerateClipTable(clip, olive::core::rational(0));
	EXPECT_EQ(probe->last_loop_mode(), olive::LoopMode::kLoopModeClamp);
}

TEST(ClipTraverser, GenerateTableDefaultsToLoopModeOff)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *probe = new LoopModeProbeNode();
	probe->setParent(&project);

	olive::NodeTraverser traverser;
	traverser.GenerateTable(
		probe, olive::TimeRange(olive::core::rational(0),
								olive::core::rational(1, 30)));
	EXPECT_EQ(probe->last_loop_mode(), olive::LoopMode::kLoopModeOff);
}

TEST(ClipTraverser, TransformAccumulatesGizmosAlongPath)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *start = new GizmoProbeNode();
	start->setParent(&project);
	start->SetGizmoTransform(QTransform().translate(1, 0));

	auto *middle = new GizmoProbeNode();
	middle->setParent(&project);
	middle->SetGizmoTransform(QTransform().translate(2, 0));

	auto *end = new GizmoProbeNode();
	end->setParent(&project);
	end->SetGizmoTransform(QTransform().translate(4, 0));

	olive::Node::ConnectEdge(start,
							 olive::NodeInput(middle, GizmoProbeNode::InputA()));
	olive::Node::ConnectEdge(middle,
							 olive::NodeInput(end, GizmoProbeNode::InputA()));

	const olive::TimeRange range(olive::core::rational(0),
								 olive::core::rational(1, 30));

	// The start node defines the reference frame, so only the gizmos of the
	// nodes between start and end (inclusive of end) are accumulated
	olive::NodeTraverser traverser;
	QTransform t;
	traverser.Transform(&t, start, end, range);
	EXPECT_DOUBLE_EQ(t.dx(), 6.0);
	EXPECT_DOUBLE_EQ(t.dy(), 0.0);

	// Stopping at the middle node accumulates only its gizmo
	olive::NodeTraverser traverser2;
	QTransform t2;
	traverser2.Transform(&t2, start, middle, range);
	EXPECT_DOUBLE_EQ(t2.dx(), 2.0);
	EXPECT_DOUBLE_EQ(t2.dy(), 0.0);
}

TEST(ClipTraverser, TransformIgnoresNodesOffThePath)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *start = new GizmoProbeNode();
	start->setParent(&project);
	start->SetGizmoTransform(QTransform().translate(1, 0));

	auto *end = new GizmoProbeNode();
	end->setParent(&project);
	end->SetGizmoTransform(QTransform().translate(4, 0));

	// Connected to the end node but not on the start->end path
	auto *off_path = new GizmoProbeNode();
	off_path->setParent(&project);
	off_path->SetGizmoTransform(QTransform().translate(100, 0));

	olive::Node::ConnectEdge(start,
							 olive::NodeInput(end, GizmoProbeNode::InputA()));
	olive::Node::ConnectEdge(off_path,
							 olive::NodeInput(end, GizmoProbeNode::InputB()));

	const olive::TimeRange range(olive::core::rational(0),
								 olive::core::rational(1, 30));

	olive::NodeTraverser traverser;
	QTransform t;
	traverser.Transform(&t, start, end, range);
	EXPECT_DOUBLE_EQ(t.dx(), 4.0);
	EXPECT_DOUBLE_EQ(t.dy(), 0.0);
}

TEST(ClipTraverser, TransformWithSameStartAndEndIsIdentity)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = new GizmoProbeNode();
	node->setParent(&project);
	node->SetGizmoTransform(QTransform().translate(4, 0));

	const olive::TimeRange range(olive::core::rational(0),
								 olive::core::rational(1, 30));

	olive::NodeTraverser traverser;
	QTransform t;
	traverser.Transform(&t, node, node, range);
	EXPECT_TRUE(t.isIdentity());
}

TEST(ClipTraverser, ViewerConnectedOutputsResolveThroughGraph)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *viewer = new olive::ViewerOutput();
	viewer->setParent(&project);

	EXPECT_EQ(viewer->GetConnectedTextureOutput(), nullptr);
	EXPECT_EQ(viewer->GetConnectedSampleOutput(), nullptr);

	auto *texture_source = new GizmoProbeNode();
	texture_source->setParent(&project);
	auto *sample_source = new GizmoProbeNode();
	sample_source->setParent(&project);

	olive::Node::ConnectEdge(
		texture_source,
		olive::NodeInput(viewer, olive::ViewerOutput::kTextureInput));
	olive::Node::ConnectEdge(
		sample_source,
		olive::NodeInput(viewer, olive::ViewerOutput::kSamplesInput));

	EXPECT_EQ(viewer->GetConnectedTextureOutput(), texture_source);
	EXPECT_EQ(viewer->GetConnectedSampleOutput(), sample_source);

	// The value hint getters delegate to the corresponding input hints
	EXPECT_EQ(viewer->GetConnectedTextureValueHint().types(),
			  viewer->GetValueHintForInput(olive::ViewerOutput::kTextureInput)
				  .types());
	EXPECT_EQ(viewer->GetConnectedSampleValueHint().types(),
			  viewer->GetValueHintForInput(olive::ViewerOutput::kSamplesInput)
				  .types());

	olive::Node::DisconnectEdge(
		texture_source,
		olive::NodeInput(viewer, olive::ViewerOutput::kTextureInput));
	EXPECT_EQ(viewer->GetConnectedTextureOutput(), nullptr);
	EXPECT_EQ(viewer->GetConnectedSampleOutput(), sample_source);
}
