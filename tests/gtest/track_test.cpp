#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>

#include <QDataStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/qtutils.h"
#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/globals.h"
#include "node/output/track/track.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "olive/core/render/samplebuffer.h"
#include "render/loopmode.h"

namespace
{

olive::Track *CreateTrack(olive::Project *project,
						  olive::Track::Type type = olive::Track::kVideo)
{
	auto *track = new olive::Track();
	track->setParent(project);
	track->set_type(type);
	return track;
}

olive::ClipBlock *CreateClip(olive::Project *project,
							 const olive::core::rational &length)
{
	auto *clip = new olive::ClipBlock();
	clip->setParent(project);
	clip->set_length_and_media_out(length);
	return clip;
}

olive::GapBlock *CreateGap(olive::Project *project,
						   const olive::core::rational &length)
{
	auto *gap = new olive::GapBlock();
	gap->setParent(project);
	gap->set_length_and_media_out(length);
	return gap;
}

float SumOfAbsoluteSamples(const olive::core::SampleBuffer &buffer)
{
	float sum = 0.0f;
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		const float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			sum += std::abs(data[i]);
		}
	}
	return sum;
}

} // namespace

TEST(Track, DefaultState)
{
	olive::Track track;

	EXPECT_EQ(track.type(), olive::Track::kNone);
	EXPECT_EQ(track.Index(), -1);
	EXPECT_TRUE(track.Blocks().isEmpty());
	EXPECT_EQ(track.track_length(), olive::core::rational(0));
	EXPECT_FALSE(track.IsMuted());
	EXPECT_FALSE(track.IsLocked());
	EXPECT_DOUBLE_EQ(track.GetTrackHeight(), olive::Track::kTrackHeightDefault);
	EXPECT_EQ(track.sequence(), nullptr);
	EXPECT_EQ(track.Name(), QStringLiteral("Track"));
	EXPECT_EQ(track.id(), QStringLiteral("org.olivevideoeditor.Olive.track"));
	EXPECT_TRUE(track.Category().contains(olive::Node::kCategoryTimeline));
}

TEST(Track, NameReflectsTypeAndIndex)
{
	olive::Track track;

	track.set_type(olive::Track::kVideo);
	track.SetIndex(2);
	EXPECT_EQ(track.Name(), QStringLiteral("Video Track 2"));

	track.set_type(olive::Track::kAudio);
	track.SetIndex(0);
	EXPECT_EQ(track.Name(), QStringLiteral("Audio Track 0"));

	track.set_type(olive::Track::kSubtitle);
	track.SetIndex(1);
	EXPECT_EQ(track.Name(), QStringLiteral("Subtitle Track 1"));

	track.set_type(olive::Track::kNone);
	EXPECT_EQ(track.Name(), QStringLiteral("Track"));
}

TEST(Track, SetIndexEmitsIndexChanged)
{
	olive::Track track;

	int emissions = 0;
	int old_index = 0;
	int new_index = 0;
	QObject::connect(&track, &olive::Track::IndexChanged,
					 [&emissions, &old_index, &new_index](int old_i, int now_i) {
						 ++emissions;
						 old_index = old_i;
						 new_index = now_i;
					 });

	track.SetIndex(3);

	EXPECT_EQ(track.Index(), 3);
	EXPECT_EQ(emissions, 1);
	EXPECT_EQ(old_index, -1);
	EXPECT_EQ(new_index, 3);
}

TEST(Track, MuteAndLockToggle)
{
	olive::Track track;

	int emissions = 0;
	bool last_muted = false;
	QObject::connect(&track, &olive::Track::MutedChanged,
					 [&emissions, &last_muted](bool e) {
						 ++emissions;
						 last_muted = e;
					 });

	track.SetMuted(true);
	EXPECT_TRUE(track.IsMuted());
	EXPECT_EQ(emissions, 1);
	EXPECT_TRUE(last_muted);

	track.SetMuted(false);
	EXPECT_FALSE(track.IsMuted());
	EXPECT_EQ(emissions, 2);
	EXPECT_FALSE(last_muted);

	EXPECT_FALSE(track.IsLocked());
	track.SetLocked(true);
	EXPECT_TRUE(track.IsLocked());
	track.SetLocked(false);
	EXPECT_FALSE(track.IsLocked());
}

TEST(Track, TrackHeightAccessors)
{
	olive::Track track;

	int emissions = 0;
	qreal last_height = 0.0;
	QObject::connect(&track, &olive::Track::TrackHeightChanged,
					 [&emissions, &last_height](qreal h) {
						 ++emissions;
						 last_height = h;
					 });

	track.SetTrackHeight(2.5);
	EXPECT_DOUBLE_EQ(track.GetTrackHeight(), 2.5);
	EXPECT_EQ(emissions, 1);
	EXPECT_DOUBLE_EQ(last_height, 2.5);

	EXPECT_DOUBLE_EQ(olive::Track::kTrackHeightDefault, 3.0);
	EXPECT_DOUBLE_EQ(olive::Track::kTrackHeightMinimum, 1.5);
	EXPECT_DOUBLE_EQ(olive::Track::kTrackHeightInterval, 0.5);
	EXPECT_LT(olive::Track::GetMinimumTrackHeightInPixels(),
			  olive::Track::GetDefaultTrackHeightInPixels());
}

TEST(Track, TrackHeightPixelRoundTrip)
{
	olive::Track track;

	track.SetTrackHeightInPixels(77);
	EXPECT_EQ(track.GetTrackHeightInPixels(), 77);

	olive::Track default_track;
	EXPECT_EQ(default_track.GetTrackHeightInPixels(),
			  olive::Track::GetDefaultTrackHeightInPixels());
}

TEST(Track, HeightSaveLoadRoundTrip)
{
	olive::Track track;
	track.SetTrackHeight(1.75);

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	track.SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	EXPECT_TRUE(xml.contains(QStringLiteral("<height>1.75</height>")));

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));

	olive::Track loaded;
	ASSERT_TRUE(loaded.LoadCustom(&reader, nullptr));
	EXPECT_DOUBLE_EQ(loaded.GetTrackHeight(), 1.75);
}

TEST(Track, ReferenceStringsRoundTrip)
{
	const olive::Track::Reference video(olive::Track::kVideo, 2);
	EXPECT_EQ(video.ToString(), QStringLiteral("v:2"));
	EXPECT_TRUE(video.IsValid());
	EXPECT_EQ(olive::Track::Reference::FromString(QStringLiteral("v:2")), video);

	const olive::Track::Reference audio(olive::Track::kAudio, 10);
	EXPECT_EQ(audio.ToString(), QStringLiteral("a:10"));
	EXPECT_EQ(olive::Track::Reference::FromString(QStringLiteral("a:10")),
			  audio);

	const olive::Track::Reference subtitle(olive::Track::kSubtitle, 0);
	EXPECT_EQ(subtitle.ToString(), QStringLiteral("s:0"));
	EXPECT_EQ(olive::Track::Reference::FromString(QStringLiteral("s:0")),
			  subtitle);

	EXPECT_EQ(olive::Track::Reference::TypeFromString(QStringLiteral("v:2")),
			  olive::Track::kVideo);
	EXPECT_EQ(olive::Track::Reference::TypeFromString(QStringLiteral("a:3")),
			  olive::Track::kAudio);
	EXPECT_EQ(olive::Track::Reference::TypeFromString(QStringLiteral("s:0")),
			  olive::Track::kSubtitle);

	EXPECT_TRUE(
		olive::Track::Reference::TypeToString(olive::Track::kNone).isEmpty());
	EXPECT_TRUE(olive::Track::Reference().ToString().isEmpty());
}

TEST(Track, ReferenceInvalidStrings)
{
	EXPECT_FALSE(
		olive::Track::Reference::FromString(QString()).IsValid());
	EXPECT_FALSE(
		olive::Track::Reference::FromString(QStringLiteral("x:1")).IsValid());
	// Too short to contain a type prefix and separator
	EXPECT_FALSE(
		olive::Track::Reference::FromString(QStringLiteral("v")).IsValid());
	// Non-numeric index fails to parse
	EXPECT_FALSE(
		olive::Track::Reference::FromString(QStringLiteral("v:x")).IsValid());
	EXPECT_EQ(
		olive::Track::Reference::TypeFromString(QStringLiteral("q:0")),
		olive::Track::kNone);

	EXPECT_FALSE(olive::Track::Reference().IsValid());
	EXPECT_FALSE(
		olive::Track::Reference(olive::Track::kCount, 0).IsValid());
	EXPECT_FALSE(
		olive::Track::Reference(olive::Track::kVideo, -1).IsValid());
}

TEST(Track, ReferenceComparisonAndHash)
{
	const olive::Track::Reference v1(olive::Track::kVideo, 1);
	const olive::Track::Reference v1_copy(olive::Track::kVideo, 1);
	const olive::Track::Reference v2(olive::Track::kVideo, 2);
	const olive::Track::Reference a1(olive::Track::kAudio, 1);

	EXPECT_EQ(v1, v1_copy);
	EXPECT_NE(v1, v2);
	EXPECT_NE(v1, a1);

	// Ordering is by type first, then index
	EXPECT_LT(v1, v2);
	EXPECT_LT(v2, a1);
	EXPECT_FALSE(a1 < v2);

	EXPECT_EQ(olive::qHash(v1), olive::qHash(v1_copy));
}

TEST(Track, ReferenceDataStreamRoundTrip)
{
	const olive::Track::Reference ref(olive::Track::kAudio, 5);

	QByteArray bytes;
	QDataStream out(&bytes, QIODevice::WriteOnly);
	out << ref;

	QDataStream in(bytes);
	olive::Track::Reference loaded;
	in >> loaded;

	EXPECT_EQ(loaded, ref);
}

TEST(Track, ToReferenceMatchesTypeAndIndex)
{
	olive::Track track;
	track.set_type(olive::Track::kAudio);
	track.SetIndex(7);

	const olive::Track::Reference ref = track.ToReference();
	EXPECT_EQ(ref, olive::Track::Reference(olive::Track::kAudio, 7));
	EXPECT_EQ(ref.ToString(), QStringLiteral("a:7"));
}

TEST(Track, AppendBlocksSetsInOutLengthAndLinks)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	int added_count = 0;
	olive::Block *last_added = nullptr;
	QObject::connect(track, &olive::Track::BlockAdded,
					 [&added_count, &last_added](olive::Block *b) {
						 ++added_count;
						 last_added = b;
					 });
	int length_changed_count = 0;
	QObject::connect(track, &olive::Track::TrackLengthChanged,
					 [&length_changed_count]() { ++length_changed_count; });

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);

	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), b);

	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));

	EXPECT_EQ(a->track(), track);
	EXPECT_EQ(b->track(), track);
	EXPECT_EQ(a->previous(), nullptr);
	EXPECT_EQ(a->next(), b);
	EXPECT_EQ(b->previous(), a);
	EXPECT_EQ(b->next(), nullptr);

	EXPECT_EQ(added_count, 2);
	EXPECT_EQ(last_added, b);
	EXPECT_EQ(length_changed_count, 2);
}

TEST(Track, PrependBlockShiftsExistingBlocks)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(2));
	track->AppendBlock(b);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(1));
	track->PrependBlock(a);

	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), b);

	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(1));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(3));

	EXPECT_EQ(a->next(), b);
	EXPECT_EQ(b->previous(), a);
}

TEST(Track, InsertBlockAtIndexMiddle)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(1));
	olive::ClipBlock *c = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(c);

	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(2));
	track->InsertBlockAtIndex(b, 1);

	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(track->Blocks().at(2), c);

	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(1));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	EXPECT_EQ(c->in(), olive::core::rational(3));
	EXPECT_EQ(c->out(), olive::core::rational(4));

	EXPECT_EQ(a->next(), b);
	EXPECT_EQ(b->previous(), a);
	EXPECT_EQ(b->next(), c);
	EXPECT_EQ(c->previous(), b);
}

TEST(Track, InsertBlockAfterAndBefore)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(1));
	olive::ClipBlock *c = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(c);

	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(2));
	track->InsertBlockAfter(b, a);

	olive::ClipBlock *d = CreateClip(&project, olive::core::rational(1));
	track->InsertBlockBefore(d, c);

	// A null "before" block prepends, a null "after" block appends
	olive::ClipBlock *e = CreateClip(&project, olive::core::rational(1));
	track->InsertBlockAfter(e, nullptr);

	olive::ClipBlock *f = CreateClip(&project, olive::core::rational(1));
	track->InsertBlockBefore(f, nullptr);

	const QVector<olive::Block *> expected = { e, a, b, d, c, f };
	ASSERT_EQ(track->Blocks().size(), expected.size());
	for (int i = 0; i < expected.size(); ++i) {
		EXPECT_EQ(track->Blocks().at(i), expected.at(i));
	}

	EXPECT_EQ(e->in(), olive::core::rational(0));
	EXPECT_EQ(a->in(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(d->in(), olive::core::rational(4));
	EXPECT_EQ(c->in(), olive::core::rational(5));
	EXPECT_EQ(f->in(), olive::core::rational(6));
	EXPECT_EQ(f->out(), olive::core::rational(7));
	EXPECT_EQ(track->track_length(), olive::core::rational(7));
}

TEST(Track, RippleRemoveBlockShiftsAndDetaches)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(3));
	olive::ClipBlock *c = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);
	ASSERT_EQ(track->track_length(), olive::core::rational(6));

	int removed_count = 0;
	olive::Block *last_removed = nullptr;
	QObject::connect(track, &olive::Track::BlockRemoved,
					 [&removed_count, &last_removed](olive::Block *blk) {
						 ++removed_count;
						 last_removed = blk;
					 });

	track->RippleRemoveBlock(b);

	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), c);

	// Subsequent blocks move earlier to fill the space
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(c->in(), olive::core::rational(2));
	EXPECT_EQ(c->out(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(3));

	// The removed block is detached and reset to zero-based in/out
	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(b->previous(), nullptr);
	EXPECT_EQ(b->next(), nullptr);
	EXPECT_EQ(b->in(), olive::core::rational(0));
	EXPECT_EQ(b->out(), b->length());

	EXPECT_EQ(a->next(), c);
	EXPECT_EQ(c->previous(), a);

	EXPECT_EQ(removed_count, 1);
	EXPECT_EQ(last_removed, b);
}

TEST(Track, RippleRemoveFirstAndLastBlock)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(3));
	olive::ClipBlock *c = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);

	track->RippleRemoveBlock(a);
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(b->in(), olive::core::rational(0));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	EXPECT_EQ(c->in(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(4));
	EXPECT_EQ(b->previous(), nullptr);

	track->RippleRemoveBlock(c);
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->track_length(), olive::core::rational(3));
	EXPECT_EQ(b->next(), nullptr);

	track->RippleRemoveBlock(b);
	EXPECT_TRUE(track->Blocks().isEmpty());
	EXPECT_EQ(track->track_length(), olive::core::rational(0));
}

TEST(Track, ReplaceBlockSameLength)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);

	int removed_count = 0;
	int added_count = 0;
	QObject::connect(track, &olive::Track::BlockRemoved,
					 [&removed_count](olive::Block *) { ++removed_count; });
	QObject::connect(track, &olive::Track::BlockAdded,
					 [&added_count](olive::Block *) { ++added_count; });

	olive::ClipBlock *r = CreateClip(&project, olive::core::rational(3));
	track->ReplaceBlock(b, r);

	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), r);

	// Same-length replacement keeps in/out points identical
	EXPECT_EQ(r->in(), olive::core::rational(2));
	EXPECT_EQ(r->out(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));

	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(b->previous(), nullptr);
	EXPECT_EQ(b->next(), nullptr);
	EXPECT_EQ(r->track(), track);
	EXPECT_EQ(r->previous(), a);
	EXPECT_EQ(r->next(), nullptr);
	EXPECT_EQ(a->next(), r);

	EXPECT_EQ(removed_count, 1);
	EXPECT_EQ(added_count, 1);
}

TEST(Track, ReplaceBlockDifferentLengthRipples)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *c = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);
	ASSERT_EQ(track->track_length(), olive::core::rational(5));

	olive::ClipBlock *r = CreateClip(&project, olive::core::rational(4));
	track->ReplaceBlock(b, r);

	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(r->in(), olive::core::rational(2));
	EXPECT_EQ(r->out(), olive::core::rational(6));
	// The longer replacement pushes subsequent blocks later
	EXPECT_EQ(c->in(), olive::core::rational(6));
	EXPECT_EQ(c->out(), olive::core::rational(7));
	EXPECT_EQ(track->track_length(), olive::core::rational(7));
}

TEST(Track, BlockLookupFunctions)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::GapBlock *g = CreateGap(&project, olive::core::rational(3));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(4));
	track->AppendBlock(a);
	track->AppendBlock(g);
	track->AppendBlock(b);
	// Layout: a [0,2], g [2,5], b [5,9]

	// BlockContainingTime: strictly inside only, never at edges
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(1)), a);
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(0)), nullptr);
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(2)), nullptr);
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(3)), g);
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(5)), nullptr);
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(8)), b);
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(9)), nullptr);
	EXPECT_EQ(track->BlockContainingTime(olive::core::rational(100)), nullptr);

	// NearestBlockBefore: block starting before and ending at/after the time
	EXPECT_EQ(track->NearestBlockBefore(olive::core::rational(2)), a);
	EXPECT_EQ(track->NearestBlockBefore(olive::core::rational(5)), g);
	EXPECT_EQ(track->NearestBlockBefore(olive::core::rational(9)), b);
	EXPECT_EQ(track->NearestBlockBefore(olive::core::rational(10)), nullptr);

	// NearestBlockBeforeOrAt: first block ending after the time
	EXPECT_EQ(track->NearestBlockBeforeOrAt(olive::core::rational(0)), a);
	EXPECT_EQ(track->NearestBlockBeforeOrAt(olive::core::rational(2)), g);
	EXPECT_EQ(track->NearestBlockBeforeOrAt(olive::core::rational(5)), b);
	EXPECT_EQ(track->NearestBlockBeforeOrAt(olive::core::rational(9)), nullptr);

	// NearestBlockAfterOrAt: first block starting at or after the time
	EXPECT_EQ(track->NearestBlockAfterOrAt(olive::core::rational(0)), a);
	EXPECT_EQ(track->NearestBlockAfterOrAt(olive::core::rational(2)), g);
	EXPECT_EQ(track->NearestBlockAfterOrAt(olive::core::rational(5)), b);
	EXPECT_EQ(track->NearestBlockAfterOrAt(olive::core::rational(9)), nullptr);

	// NearestBlockAfter: first block starting strictly after the time
	EXPECT_EQ(track->NearestBlockAfter(olive::core::rational(0)), g);
	EXPECT_EQ(track->NearestBlockAfter(olive::core::rational(2)), b);
	EXPECT_EQ(track->NearestBlockAfter(olive::core::rational(4)), b);
	EXPECT_EQ(track->NearestBlockAfter(olive::core::rational(5)), nullptr);

	// VisibleBlockAtTime: half-open [in, out) containment via binary search
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(0)), a);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(1)), a);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(2)), g);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(4)), g);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(5)), b);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(8)), b);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(9)), nullptr);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(-1)), nullptr);
	EXPECT_EQ(track->VisibleBlockAtTime(olive::core::rational(100)), nullptr);
}

TEST(Track, BlockLookupOnEmptyTrack)
{
	olive::Track track;

	EXPECT_EQ(track.BlockContainingTime(olive::core::rational(0)), nullptr);
	EXPECT_EQ(track.NearestBlockBefore(olive::core::rational(0)), nullptr);
	EXPECT_EQ(track.NearestBlockBeforeOrAt(olive::core::rational(0)), nullptr);
	EXPECT_EQ(track.NearestBlockAfterOrAt(olive::core::rational(0)), nullptr);
	EXPECT_EQ(track.NearestBlockAfter(olive::core::rational(0)), nullptr);
	EXPECT_EQ(track.VisibleBlockAtTime(olive::core::rational(0)), nullptr);
	EXPECT_EQ(track.track_length(), olive::core::rational(0));
}

TEST(Track, IsRangeFree)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::GapBlock *g = CreateGap(&project, olive::core::rational(3));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(4));
	track->AppendBlock(a);
	track->AppendBlock(g);
	track->AppendBlock(b);
	// Layout: clip a [0,2], gap g [2,5], clip b [5,9]

	// A range covering only the gap (or part of it) is free
	EXPECT_TRUE(track->IsRangeFree(olive::core::TimeRange(
		olive::core::rational(2), olive::core::rational(5))));
	EXPECT_TRUE(track->IsRangeFree(olive::core::TimeRange(
		olive::core::rational(3), olive::core::rational(4))));

	// Ranges touching clips are not free
	EXPECT_FALSE(track->IsRangeFree(olive::core::TimeRange(
		olive::core::rational(0), olive::core::rational(2))));
	EXPECT_FALSE(track->IsRangeFree(olive::core::TimeRange(
		olive::core::rational(1), olive::core::rational(3))));
	EXPECT_FALSE(track->IsRangeFree(olive::core::TimeRange(
		olive::core::rational(4), olive::core::rational(9))));

	// Past the end of the track there is nothing in the way
	EXPECT_TRUE(track->IsRangeFree(olive::core::TimeRange(
		olive::core::rational(9), olive::core::rational(12))));

	olive::Track empty_track;
	EXPECT_TRUE(empty_track.IsRangeFree(olive::core::TimeRange(
		olive::core::rational(0), olive::core::rational(10))));
}

TEST(Track, ArrayIndexesAreReusedAfterRemoval)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(1));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(1));
	olive::ClipBlock *c = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);

	EXPECT_EQ(track->GetArrayIndexFromBlock(a), 0);
	EXPECT_EQ(track->GetArrayIndexFromBlock(b), 1);
	EXPECT_EQ(track->GetArrayIndexFromBlock(c), 2);

	// Removing B frees its input array index; the next block reuses it
	track->RippleRemoveBlock(b);

	olive::ClipBlock *d = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(d);

	EXPECT_EQ(track->GetArrayIndexFromBlock(a), 0);
	EXPECT_EQ(track->GetArrayIndexFromBlock(c), 2);
	EXPECT_EQ(track->GetArrayIndexFromBlock(d), 1);

	// The array map standard value mirrors the cache order [a, c, d]
	const QByteArray bytes =
		track->GetStandardValue(olive::Track::kArrayMapInput).toByteArray();
	ASSERT_EQ(bytes.size(), 3 * int(sizeof(uint32_t)));
	uint32_t values[3];
	std::memcpy(values, bytes.constData(), sizeof(values));
	EXPECT_EQ(values[0], 0u);
	EXPECT_EQ(values[1], 2u);
	EXPECT_EQ(values[2], 1u);
}

TEST(Track, BlockLengthChangeRipplesSubsequentBlocks)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(b);
	ASSERT_EQ(track->track_length(), olive::core::rational(4));

	int refreshed_count = 0;
	QObject::connect(track, &olive::Track::BlocksRefreshed,
					 [&refreshed_count]() { ++refreshed_count; });

	a->set_length_and_media_out(olive::core::rational(5));
	EXPECT_EQ(a->out(), olive::core::rational(5));
	EXPECT_EQ(b->in(), olive::core::rational(5));
	EXPECT_EQ(b->out(), olive::core::rational(7));
	EXPECT_EQ(track->track_length(), olive::core::rational(7));

	b->set_length_and_media_out(olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(5));
	EXPECT_EQ(b->out(), olive::core::rational(6));
	EXPECT_EQ(track->track_length(), olive::core::rational(6));

	EXPECT_EQ(refreshed_count, 2);
}

TEST(Track, GetActiveElementsAtTime)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	// An empty track has no active elements
	EXPECT_EQ(track->GetActiveElementsAtTime(
					  olive::Track::kBlockInput,
					  olive::core::TimeRange(olive::core::rational(0),
											 olive::core::rational(1)))
					  .mode(),
				  olive::Node::ActiveElements::kNoElements);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::GapBlock *g = CreateGap(&project, olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(g);
	// Layout: clip a [0,2], gap g [2,4]

	// The clip is the only active element in [0,1]
	const olive::Node::ActiveElements active =
		track->GetActiveElementsAtTime(
			olive::Track::kBlockInput,
			olive::core::TimeRange(olive::core::rational(0),
								   olive::core::rational(1)));
	EXPECT_EQ(active.mode(), olive::Node::ActiveElements::kSpecified);
	ASSERT_EQ(active.elements().size(), 1);
	EXPECT_EQ(active.elements().front(), track->GetArrayIndexFromBlock(a));

	// Gaps never count as active elements
	EXPECT_EQ(track->GetActiveElementsAtTime(
					  olive::Track::kBlockInput,
					  olive::core::TimeRange(olive::core::rational(2),
											 olive::core::rational(4)))
					  .mode(),
				  olive::Node::ActiveElements::kNoElements);

	// A muted track reports no active elements
	track->SetMuted(true);
	EXPECT_EQ(track->GetActiveElementsAtTime(
					  olive::Track::kBlockInput,
					  olive::core::TimeRange(olive::core::rational(0),
											 olive::core::rational(1)))
					  .mode(),
				  olive::Node::ActiveElements::kNoElements);
	track->SetMuted(false);

	// A disabled clip is not an active element
	a->set_enabled(false);
	EXPECT_EQ(track->GetActiveElementsAtTime(
					  olive::Track::kBlockInput,
					  olive::core::TimeRange(olive::core::rational(0),
											 olive::core::rational(1)))
					  .mode(),
				  olive::Node::ActiveElements::kNoElements);

	// Ranges outside the track length have no active elements
	EXPECT_EQ(track->GetActiveElementsAtTime(
					  olive::Track::kBlockInput,
					  olive::core::TimeRange(olive::core::rational(4),
											 olive::core::rational(8)))
					  .mode(),
				  olive::Node::ActiveElements::kNoElements);
	EXPECT_EQ(track->GetActiveElementsAtTime(
					  olive::Track::kBlockInput,
					  olive::core::TimeRange(olive::core::rational(-1),
											 olive::core::rational(0)))
					  .mode(),
				  olive::Node::ActiveElements::kNoElements);

	// Non-block inputs fall back to the default (all elements)
	EXPECT_EQ(track->GetActiveElementsAtTime(
					  olive::Track::kMutedInput,
					  olive::core::TimeRange(olive::core::rational(0),
											 olive::core::rational(1)))
					  .mode(),
				  olive::Node::ActiveElements::kAllElements);
}

TEST(Track, TimeAdjustmentTransformsAroundBlock)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	const int b_index = track->GetArrayIndexFromBlock(b);

	// Without clamping, the range is simply shifted by the block's in point
	EXPECT_EQ(track->InputTimeAdjustment(
				  olive::Track::kBlockInput, b_index,
				  olive::core::TimeRange(olive::core::rational(2),
										 olive::core::rational(5)),
				  false),
			  olive::core::TimeRange(olive::core::rational(0),
									 olive::core::rational(3)));

	// Clamping limits the range to the block before shifting
	EXPECT_EQ(track->InputTimeAdjustment(
				  olive::Track::kBlockInput, b_index,
				  olive::core::TimeRange(olive::core::rational(0),
										 olive::core::rational(10)),
				  true),
			  olive::core::TimeRange(olive::core::rational(0),
									 olive::core::rational(3)));
	EXPECT_EQ(track->InputTimeAdjustment(
				  olive::Track::kBlockInput, b_index,
				  olive::core::TimeRange(olive::core::rational(1),
										 olive::core::rational(4)),
				  true),
			  olive::core::TimeRange(olive::core::rational(0),
									 olive::core::rational(2)));

	// Output adjustment shifts block-local time back into track time
	EXPECT_EQ(track->OutputTimeAdjustment(
				  olive::Track::kBlockInput, b_index,
				  olive::core::TimeRange(olive::core::rational(0),
										 olive::core::rational(3))),
			  olive::core::TimeRange(olive::core::rational(2),
									 olive::core::rational(5)));

	// Unknown elements and inputs pass the range through unchanged
	const olive::core::TimeRange unchanged(olive::core::rational(1),
										   olive::core::rational(2));
	EXPECT_EQ(track->InputTimeAdjustment(olive::Track::kBlockInput, 99,
										 unchanged, true),
			  unchanged);
	EXPECT_EQ(track->OutputTimeAdjustment(olive::Track::kBlockInput, 99,
										  unchanged),
			  unchanged);
	EXPECT_EQ(track->InputTimeAdjustment(olive::Track::kMutedInput, -1,
										 unchanged, true),
			  unchanged);
}

TEST(Track, TransformTimeHelpersRespectInfinities)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// b occupies [2,5]

	const olive::core::rational kMax(INT_MAX);
	const olive::core::rational kMin(INT_MIN);

	EXPECT_EQ(olive::Track::TransformTimeForBlock(b, kMax), kMax);
	EXPECT_EQ(olive::Track::TransformTimeForBlock(b, kMin), kMin);
	EXPECT_EQ(olive::Track::TransformTimeFromBlock(b, kMax), kMax);
	EXPECT_EQ(olive::Track::TransformTimeFromBlock(b, kMin), kMin);

	EXPECT_EQ(olive::Track::TransformTimeForBlock(b, olive::core::rational(3)),
			  olive::core::rational(1));
	EXPECT_EQ(
		olive::Track::TransformTimeFromBlock(b, olive::core::rational(1)),
		olive::core::rational(3));

	EXPECT_EQ(olive::Track::TransformRangeForBlock(
				  b, olive::core::TimeRange(olive::core::rational(2),
											olive::core::rational(5))),
			  olive::core::TimeRange(olive::core::rational(0),
									 olive::core::rational(3)));
	EXPECT_EQ(olive::Track::TransformRangeFromBlock(
				  b, olive::core::TimeRange(olive::core::rational(0),
											olive::core::rational(3))),
			  olive::core::TimeRange(olive::core::rational(2),
									 olive::core::rational(5)));
}

TEST(Track, VideoValuePassesThroughFirstArrayElement)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project, olive::Track::kVideo);

	olive::NodeValueArray arr;
	arr.emplace(0, olive::NodeValue(olive::NodeValue::kFloat, 42.0));

	olive::NodeValueRow row;
	row.insert(olive::Track::kBlockInput,
			   olive::NodeValue(olive::NodeValue::kNone, arr, nullptr, true));

	olive::NodeValueTable table;
	track->Value(row, olive::NodeGlobals(), &table);

	const olive::NodeValue out = table.Get(olive::NodeValue::kFloat);
	ASSERT_EQ(out.type(), olive::NodeValue::kFloat);
	EXPECT_DOUBLE_EQ(out.toDouble(), 42.0);

	// An empty block array pushes nothing
	olive::NodeValueRow empty_row;
	empty_row.insert(olive::Track::kBlockInput,
					 olive::NodeValue(olive::NodeValue::kNone,
									  olive::NodeValueArray(), nullptr, true));

	olive::NodeValueTable empty_table;
	track->Value(empty_row, olive::NodeGlobals(), &empty_table);
	EXPECT_EQ(empty_table.Get(olive::NodeValue::kFloat).type(),
			  olive::NodeValue::kNone);
}

TEST(Track, AudioValueProducesSilentBufferWithoutBlocks)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project, olive::Track::kAudio);

	const olive::core::AudioParams aparams(48000,
										   olive::core::kChannelLayoutStereo,
										   olive::core::SampleFormat::F32P);

	olive::NodeValueRow row;
	row.insert(olive::Track::kBlockInput,
			   olive::NodeValue(olive::NodeValue::kNone,
								olive::NodeValueArray(), nullptr, true));

	const olive::NodeGlobals globals(
		olive::VideoParams(), aparams,
		olive::core::TimeRange(olive::core::rational(0),
							   olive::core::rational(1, 2)),
		olive::LoopMode::kLoopModeOff);

	olive::NodeValueTable table;
	track->Value(row, globals, &table);

	const olive::NodeValue out = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(out.type(), olive::NodeValue::kSamples);

	const olive::core::SampleBuffer samples = out.toSamples();
	ASSERT_TRUE(samples.is_allocated());
	EXPECT_EQ(samples.channel_count(), 2);
	EXPECT_EQ(samples.sample_count(), 24000);
	EXPECT_FLOAT_EQ(SumOfAbsoluteSamples(samples), 0.0f);
}

TEST(Track, AudioValueMixesBlockSamples)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project, olive::Track::kAudio);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(1));
	olive::ClipBlock *b = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,1], b [1,2]

	const olive::core::AudioParams aparams(48000,
										   olive::core::kChannelLayoutStereo,
										   olive::core::SampleFormat::F32P);

	olive::core::SampleBuffer a_samples(aparams, olive::core::rational(1));
	olive::core::SampleBuffer b_samples(aparams, olive::core::rational(1));
	ASSERT_TRUE(a_samples.is_allocated());
	ASSERT_TRUE(b_samples.is_allocated());
	for (int ch = 0; ch < a_samples.channel_count(); ++ch) {
		std::fill(a_samples.data(ch), a_samples.data(ch) + 48000, 1.0f);
		std::fill(b_samples.data(ch), b_samples.data(ch) + 48000, 0.5f);
	}

	olive::NodeValueArray arr;
	arr.emplace(track->GetArrayIndexFromBlock(a),
				olive::NodeValue(olive::NodeValue::kSamples, a_samples));
	arr.emplace(track->GetArrayIndexFromBlock(b),
				olive::NodeValue(olive::NodeValue::kSamples, b_samples));

	olive::NodeValueRow row;
	row.insert(olive::Track::kBlockInput,
			   olive::NodeValue(olive::NodeValue::kNone, arr, nullptr, true));

	const olive::NodeGlobals globals(
		olive::VideoParams(), aparams,
		olive::core::TimeRange(olive::core::rational(0),
							   olive::core::rational(2)),
		olive::LoopMode::kLoopModeOff);

	olive::NodeValueTable table;
	track->Value(row, globals, &table);

	const olive::NodeValue out = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(out.type(), olive::NodeValue::kSamples);

	const olive::core::SampleBuffer mixed = out.toSamples();
	ASSERT_TRUE(mixed.is_allocated());
	ASSERT_EQ(mixed.sample_count(), 96000);

	// Each block's samples land at their track position
	EXPECT_FLOAT_EQ(mixed.data(0)[0], 1.0f);
	EXPECT_FLOAT_EQ(mixed.data(0)[47999], 1.0f);
	EXPECT_FLOAT_EQ(mixed.data(0)[48000], 0.5f);
	EXPECT_FLOAT_EQ(mixed.data(0)[95999], 0.5f);
	EXPECT_FLOAT_EQ(mixed.data(1)[24000], 1.0f);
	EXPECT_FLOAT_EQ(mixed.data(1)[72000], 0.5f);
}

TEST(Track, AudioValueSilencesZeroSpeedClip)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project, olive::Track::kAudio);

	olive::ClipBlock *a = CreateClip(&project, olive::core::rational(1));
	track->AppendBlock(a);
	a->SetStandardValue(olive::ClipBlock::kSpeedInput, 0.0);
	ASSERT_DOUBLE_EQ(a->speed(), 0.0);

	const olive::core::AudioParams aparams(48000,
										   olive::core::kChannelLayoutStereo,
										   olive::core::SampleFormat::F32P);

	olive::core::SampleBuffer a_samples(aparams, olive::core::rational(1));
	ASSERT_TRUE(a_samples.is_allocated());
	for (int ch = 0; ch < a_samples.channel_count(); ++ch) {
		std::fill(a_samples.data(ch), a_samples.data(ch) + 48000, 1.0f);
	}

	olive::NodeValueArray arr;
	arr.emplace(track->GetArrayIndexFromBlock(a),
				olive::NodeValue(olive::NodeValue::kSamples, a_samples));

	olive::NodeValueRow row;
	row.insert(olive::Track::kBlockInput,
			   olive::NodeValue(olive::NodeValue::kNone, arr, nullptr, true));

	const olive::NodeGlobals globals(
		olive::VideoParams(), aparams,
		olive::core::TimeRange(olive::core::rational(0),
							   olive::core::rational(1)),
		olive::LoopMode::kLoopModeOff);

	olive::NodeValueTable table;
	track->Value(row, globals, &table);

	const olive::NodeValue out = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(out.type(), olive::NodeValue::kSamples);

	// Zero-speed audio is defined to come out as silence
	const olive::core::SampleBuffer samples = out.toSamples();
	ASSERT_TRUE(samples.is_allocated());
	EXPECT_FLOAT_EQ(SumOfAbsoluteSamples(samples), 0.0f);
}

TEST(Track, SubtitleValuePushesNothing)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Track *track = CreateTrack(&project, olive::Track::kSubtitle);

	olive::NodeValueArray arr;
	arr.emplace(0, olive::NodeValue(olive::NodeValue::kFloat, 42.0));

	olive::NodeValueRow row;
	row.insert(olive::Track::kBlockInput,
			   olive::NodeValue(olive::NodeValue::kNone, arr, nullptr, true));

	olive::NodeValueTable table;
	track->Value(row, olive::NodeGlobals(), &table);
	EXPECT_EQ(table.Count(), 0);
}
