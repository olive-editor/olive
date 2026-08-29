#include <gtest/gtest.h>

#include <cstring>
#include <memory>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QUuid>
#include <QVector>

#include "codec/proxymanager.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/group/group.h"
#include "node/math/math/math.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "render/audioplaybackcache.h"
#include "render/diskmanager.h"
#include "render/playbackcache.h"
#include "render/projectcopier.h"

namespace
{

// Exposes the protected Validate() hook and records the virtual event
// callbacks so the invalidate/persist paths can be observed from the tests.
class TestPlaybackCache : public olive::PlaybackCache {
public:
	explicit TestPlaybackCache(QObject *parent = nullptr)
		: olive::PlaybackCache(parent)
	{
	}

	using olive::PlaybackCache::Validate;

	int invalidate_event_count = 0;
	int load_state_event_count = 0;
	int save_state_event_count = 0;

protected:
	virtual void InvalidateEvent(const olive::TimeRange &) override
	{
		invalidate_event_count++;
	}

	virtual void LoadStateEvent(QDataStream &) override
	{
		load_state_event_count++;
	}

	virtual void SaveStateEvent(QDataStream &) override
	{
		save_state_event_count++;
	}
};

} // namespace

class RenderCopierTestBase : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!temp_dir_.isValid()) {
			GTEST_FAIL() << "Failed to create temporary directory";
		}

		olive::ColorManager::SetUpDefaultConfig();

		if (!olive::Core::instance()) {
			// Leaked intentionally: Core is process-wide (matches
			// render_diskcache_test)
			new olive::Core(olive::Core::CoreParams());
		}

		olive::DiskManager::CreateInstance();

		// Point the project cache at a folder alongside the (unsaved) project
		// file so every cache read/write stays inside the temporary directory.
		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
		project_->set_filename(
			QDir(temp_dir_.path()).filePath(QStringLiteral("test.ove")));
		project_->SetCacheLocationSetting(
			olive::Project::kCacheStoreAlongsideProject);
	}

	void TearDown() override
	{
		copier_.reset();
		project_.reset();
		olive::DiskManager::DestroyInstance();
	}

	template <typename T> T *AddNode()
	{
		T *node = new T();
		node->setParent(project_.get());
		return node;
	}

	QString CacheRoot() const
	{
		return QDir(temp_dir_.path()).filePath(QStringLiteral("cache"));
	}

	QTemporaryDir temp_dir_;
	std::unique_ptr<olive::Project> project_;
	std::unique_ptr<olive::ProjectCopier> copier_;
};

class RenderProjectCopierTest : public RenderCopierTestBase {
};

class RenderPlaybackCacheTest : public RenderCopierTestBase {
};

TEST_F(RenderProjectCopierTest, CopyIsSeparateProjectWithSameStructure)
{
	auto *math_a = AddNode<olive::MathNode>();
	auto *math_b = AddNode<olive::MathNode>();
	AddNode<olive::SolidGenerator>();

	olive::Node::ConnectEdge(
		math_a, olive::NodeInput(math_b, olive::MathNode::kParamAIn));

	project_->SetSetting(QStringLiteral("copier_test_key"),
						 QStringLiteral("copier_test_value"));

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	olive::Project *copy = copier_->GetCopiedProject();
	ASSERT_NE(copy, nullptr);
	EXPECT_NE(copy, project_.get());

	// The copy is marked as an in-memory render proxy
	EXPECT_TRUE(copy->property("_oak_render_proxy").toBool());

	// Same number of nodes, same IDs, different objects
	ASSERT_EQ(copy->nodes().size(), project_->nodes().size());
	EXPECT_EQ(copier_->GetNodeMap().size(), project_->nodes().size());
	for (olive::Node *original : project_->nodes()) {
		olive::Node *cloned = copier_->GetCopy(original);
		ASSERT_NE(cloned, nullptr);
		EXPECT_NE(cloned, original);
		EXPECT_EQ(cloned->id(), original->id());
		EXPECT_EQ(copier_->GetOriginal(cloned), original);
		EXPECT_TRUE(copy->nodes().contains(cloned));
	}

	// Settings were copied
	EXPECT_EQ(copy->GetSetting(QStringLiteral("copier_test_key")),
			  QStringLiteral("copier_test_value"));

	// The pre-existing edge was recreated between the copies
	olive::Node *copy_a = copier_->GetCopy(math_a);
	olive::Node *copy_b = copier_->GetCopy(math_b);
	ASSERT_EQ(copy_b->input_connections().size(), 1);
	EXPECT_EQ(copy_b->input_connections().at(
				  olive::NodeInput(copy_b, olive::MathNode::kParamAIn)),
			  copy_a);

	// SetProject applies the initial sync synchronously
	EXPECT_FALSE(copier_->HasUpdatesInQueue());
}

TEST_F(RenderProjectCopierTest, CopiedNodesHaveDisabledCachesAndSharedUuids)
{
	auto *math = AddNode<olive::MathNode>();

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	olive::Node *copy = copier_->GetCopy(math);
	ASSERT_NE(copy, nullptr);

	// Caches are disabled on the render-proxy copy but keep the same UUIDs so
	// the copy can read frames written by the original
	EXPECT_TRUE(math->AreCachesEnabled());
	EXPECT_FALSE(copy->AreCachesEnabled());
	EXPECT_EQ(copy->video_frame_cache()->GetUuid(),
			  math->video_frame_cache()->GetUuid());
	EXPECT_EQ(copy->audio_playback_cache()->GetUuid(),
			  math->audio_playback_cache()->GetUuid());
}

TEST_F(RenderProjectCopierTest, AddedNodeSignalFiresForEachCopiedNode)
{
	AddNode<olive::MathNode>();
	AddNode<olive::SolidGenerator>();

	copier_ = std::make_unique<olive::ProjectCopier>();

	QVector<olive::Node *> added;
	QObject::connect(copier_.get(), &olive::ProjectCopier::AddedNode,
					 [&added](olive::Node *n) { added.append(n); });

	copier_->SetProject(project_.get());

	// One signal per node in the original project (color manager, root folder,
	// and the two nodes added above), carrying the *original* pointers
	EXPECT_EQ(added.size(), project_->nodes().size());
	for (olive::Node *n : project_->nodes()) {
		EXPECT_TRUE(added.contains(n));
	}
}

TEST_F(RenderProjectCopierTest, QueuedNodeAddIsAppliedByProcessUpdateQueue)
{
	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	QVector<olive::Node *> added;
	QObject::connect(copier_.get(), &olive::ProjectCopier::AddedNode,
					 [&added](olive::Node *n) { added.append(n); });

	auto *math = AddNode<olive::MathNode>();

	// The change is queued, not applied immediately
	EXPECT_TRUE(copier_->HasUpdatesInQueue());
	EXPECT_EQ(copier_->GetCopy(math), nullptr);
	EXPECT_TRUE(added.isEmpty());

	copier_->ProcessUpdateQueue();

	EXPECT_FALSE(copier_->HasUpdatesInQueue());
	olive::Node *copy = copier_->GetCopy(math);
	ASSERT_NE(copy, nullptr);
	EXPECT_EQ(copy->id(), math->id());
	EXPECT_TRUE(copier_->GetCopiedProject()->nodes().contains(copy));
	ASSERT_EQ(added.size(), 1);
	EXPECT_EQ(added.first(), math);

	// Processing the queue marked the copy as modified
	EXPECT_TRUE(copier_->GetCopiedProject()->is_modified());
}

TEST_F(RenderProjectCopierTest, QueuedNodeRemoveDeletesCopy)
{
	auto *math = AddNode<olive::MathNode>();

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	olive::Node *copy = copier_->GetCopy(math);
	ASSERT_NE(copy, nullptr);

	QVector<olive::Node *> removed;
	QObject::connect(copier_.get(), &olive::ProjectCopier::RemovedNode,
					 [&removed](olive::Node *n) { removed.append(n); });

	delete math;
	EXPECT_TRUE(copier_->HasUpdatesInQueue());

	copier_->ProcessUpdateQueue();

	EXPECT_FALSE(copier_->HasUpdatesInQueue());
	ASSERT_EQ(removed.size(), 1);
	EXPECT_EQ(removed.first(), math);
	EXPECT_EQ(copier_->GetCopy(math), nullptr);
	EXPECT_FALSE(copier_->GetCopiedProject()->nodes().contains(copy));
}

TEST_F(RenderProjectCopierTest, QueuedEdgeAddAndRemoveAreMirrored)
{
	auto *src = AddNode<olive::MathNode>();
	auto *dst = AddNode<olive::MathNode>();

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	olive::Node *copy_src = copier_->GetCopy(src);
	olive::Node *copy_dst = copier_->GetCopy(dst);
	ASSERT_NE(copy_src, nullptr);
	ASSERT_NE(copy_dst, nullptr);

	olive::Node::ConnectEdge(
		src, olive::NodeInput(dst, olive::MathNode::kParamAIn));
	EXPECT_TRUE(copier_->HasUpdatesInQueue());
	EXPECT_TRUE(copy_dst->input_connections().empty());

	copier_->ProcessUpdateQueue();

	ASSERT_EQ(copy_dst->input_connections().size(), 1);
	EXPECT_EQ(copy_dst->input_connections().at(
				  olive::NodeInput(copy_dst, olive::MathNode::kParamAIn)),
			  copy_src);

	olive::Node::DisconnectEdge(
		src, olive::NodeInput(dst, olive::MathNode::kParamAIn));
	copier_->ProcessUpdateQueue();

	EXPECT_TRUE(copy_dst->input_connections().empty());
	EXPECT_TRUE(copy_src->output_connections().empty());
}

TEST_F(RenderProjectCopierTest, QueuedValueChangeKeepsCopyIndependentUntilProcessed)
{
	auto *math = AddNode<olive::MathNode>();

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	olive::Node *copy = copier_->GetCopy(math);
	ASSERT_NE(copy, nullptr);

	const double before =
		math->GetStandardValue(olive::MathNode::kParamAIn).toDouble();
	const double changed = before + 2.5;
	math->SetStandardValue(olive::MathNode::kParamAIn, changed);

	EXPECT_TRUE(copier_->HasUpdatesInQueue());
	// The copy must not change until the queue is processed
	EXPECT_DOUBLE_EQ(
		copy->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), before);

	copier_->ProcessUpdateQueue();

	EXPECT_DOUBLE_EQ(
		copy->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), changed);
	EXPECT_DOUBLE_EQ(
		math->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), changed);
}

TEST_F(RenderProjectCopierTest, QueuedValueHintChangeIsMirrored)
{
	auto *math = AddNode<olive::MathNode>();

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	olive::Node *copy = copier_->GetCopy(math);
	ASSERT_NE(copy, nullptr);

	const olive::Node::ValueHint hint({ olive::NodeValue::kFloat }, 7,
									  QStringLiteral("copier_hint"));
	math->SetValueHintForInput(olive::MathNode::kParamAIn, hint);
	EXPECT_TRUE(copier_->HasUpdatesInQueue());

	copier_->ProcessUpdateQueue();

	const olive::Node::ValueHint copied_hint =
		copy->GetValueHintForInput(olive::MathNode::kParamAIn);
	EXPECT_EQ(copied_hint.tag(), QStringLiteral("copier_hint"));
	EXPECT_EQ(copied_hint.index(), 7);
	ASSERT_EQ(copied_hint.types().size(), 1);
	EXPECT_EQ(copied_hint.types().first(), olive::NodeValue::kFloat);
}

TEST_F(RenderProjectCopierTest, QueuedProjectSettingChangeIsMirrored)
{
	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	project_->SetSetting(QStringLiteral("copier_late_key"),
						 QStringLiteral("copier_late_value"));
	EXPECT_TRUE(copier_->HasUpdatesInQueue());
	EXPECT_TRUE(copier_->GetCopiedProject()
					->GetSetting(QStringLiteral("copier_late_key"))
					.isEmpty());

	copier_->ProcessUpdateQueue();

	EXPECT_EQ(copier_->GetCopiedProject()->GetSetting(
				  QStringLiteral("copier_late_key")),
			  QStringLiteral("copier_late_value"));
}

TEST_F(RenderProjectCopierTest, GroupNodesAreNotCopied)
{
	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	const int copy_count_before =
		copier_->GetCopiedProject()->nodes().size();

	auto *group = AddNode<olive::NodeGroup>();
	copier_->ProcessUpdateQueue();

	// Group nodes are dummies for rendering and must not appear in the copy
	EXPECT_EQ(copier_->GetCopy(group), nullptr);
	EXPECT_EQ(copier_->GetCopiedProject()->nodes().size(), copy_count_before);
	for (olive::Node *n : copier_->GetCopiedProject()->nodes()) {
		EXPECT_NE(n->id(), group->id());
	}
}

TEST_F(RenderProjectCopierTest, GraphChangeTimeAdvancesAndSyncCatchesUp)
{
	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	// SetProject leaves the sync point at or after the graph change point
	EXPECT_GE(copier_->GetLastUpdateTime().value(),
			  copier_->GetGraphChangeTime().value());

	AddNode<olive::MathNode>();

	// A queued change moves the graph change point ahead of the sync point
	EXPECT_GT(copier_->GetGraphChangeTime().value(),
			  copier_->GetLastUpdateTime().value());

	copier_->ProcessUpdateQueue();

	EXPECT_GE(copier_->GetLastUpdateTime().value(),
			  copier_->GetGraphChangeTime().value());
}

TEST_F(RenderProjectCopierTest, FootageProxySettingsSyncToCopyImmediately)
{
	auto *footage = AddNode<olive::Footage>();

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	olive::Footage *copy = copier_->GetCopy(footage);
	ASSERT_NE(copy, nullptr);
	EXPECT_FALSE(copy->proxy_enabled());

	// Proxy settings are not Node inputs, so the copier mirrors them through a
	// direct connection without involving the update queue
	footage->SetProxy(QStringLiteral("/cache/proxy/example.mp4"),
					  olive::ProxyManager::kProxyReady, 2, 3, true);

	EXPECT_FALSE(copier_->HasUpdatesInQueue());
	EXPECT_TRUE(copy->proxy_enabled());
	EXPECT_EQ(copy->proxy_path(), QStringLiteral("/cache/proxy/example.mp4"));
	EXPECT_EQ(copy->proxy_state(), olive::ProxyManager::kProxyReady);
	EXPECT_EQ(copy->proxy_video_stream_index(), 2);
	EXPECT_EQ(copy->proxy_preset_version(), 3);

	footage->SetProxy(QString(), olive::ProxyManager::kProxyMissing, -1, 0,
					  false);
	EXPECT_FALSE(copy->proxy_enabled());
	EXPECT_EQ(copy->proxy_state(), olive::ProxyManager::kProxyMissing);
}

TEST_F(RenderProjectCopierTest, SetProjectTwiceReplacesCopyContents)
{
	auto *first = AddNode<olive::MathNode>();

	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());
	ASSERT_NE(copier_->GetCopy(first), nullptr);

	olive::Project second_project;
	second_project.Initialize();
	auto *second = new olive::SolidGenerator();
	second->setParent(&second_project);

	copier_->SetProject(&second_project);

	// Nodes from the first project are gone, nodes from the second are present
	EXPECT_EQ(copier_->GetCopy(first), nullptr);
	EXPECT_NE(copier_->GetCopy(second), nullptr);
	EXPECT_EQ(copier_->GetCopiedProject()->nodes().size(),
			  second_project.nodes().size());
	EXPECT_FALSE(copier_->HasUpdatesInQueue());
}

TEST_F(RenderProjectCopierTest, SetProjectNullStopsTracking)
{
	copier_ = std::make_unique<olive::ProjectCopier>();
	copier_->SetProject(project_.get());

	AddNode<olive::MathNode>();
	EXPECT_TRUE(copier_->HasUpdatesInQueue());

	copier_->SetProject(nullptr);

	// Pending changes are discarded and the original is no longer tracked
	EXPECT_FALSE(copier_->HasUpdatesInQueue());
	EXPECT_NE(copier_->GetCopiedProject(), nullptr);

	AddNode<olive::SolidGenerator>();
	EXPECT_FALSE(copier_->HasUpdatesInQueue());
}

TEST_F(RenderPlaybackCacheTest, UuidAndSavingFlagAccessors)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);

	EXPECT_FALSE(cache.GetUuid().isNull());
	EXPECT_EQ(cache.parent(), node);
	EXPECT_TRUE(cache.IsSavingEnabled());
	EXPECT_NE(cache.mutex(), nullptr);

	TestPlaybackCache other(node);
	EXPECT_NE(other.GetUuid(), cache.GetUuid());

	const QUuid uuid = QUuid::createUuid();
	cache.SetUuid(uuid);
	EXPECT_EQ(cache.GetUuid(), uuid);

	cache.SetSavingEnabled(false);
	EXPECT_FALSE(cache.IsSavingEnabled());
}

TEST_F(RenderPlaybackCacheTest, ValidateAndInvalidateBookkeeping)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);

	QVector<olive::TimeRange> validated_signals;
	QVector<olive::TimeRange> invalidated_signals;
	QObject::connect(&cache, &olive::PlaybackCache::Validated,
					 [&validated_signals](const olive::TimeRange &r) {
						 validated_signals.append(r);
					 });
	QObject::connect(&cache, &olive::PlaybackCache::Invalidated,
					 [&invalidated_signals](const olive::TimeRange &r) {
						 invalidated_signals.append(r);
					 });

	const olive::TimeRange whole(olive::rational(0), olive::rational(10));
	EXPECT_TRUE(cache.HasInvalidatedRanges(whole));
	EXPECT_FALSE(cache.HasValidatedRanges());

	cache.Validate(whole);

	EXPECT_TRUE(cache.HasValidatedRanges());
	EXPECT_TRUE(cache.GetValidatedRanges().contains(whole));
	EXPECT_FALSE(cache.HasInvalidatedRanges(whole));
	EXPECT_TRUE(cache.GetInvalidatedRanges(olive::rational(10)).isEmpty());
	EXPECT_EQ(cache.invalidate_event_count, 0);
	ASSERT_EQ(validated_signals.size(), 1);
	EXPECT_EQ(validated_signals.first(), whole);

	// A larger query range still reports the remainder as invalidated
	EXPECT_TRUE(cache.HasInvalidatedRanges(
		olive::TimeRange(olive::rational(0), olive::rational(11))));

	const olive::TimeRange hole(olive::rational(2), olive::rational(5));
	cache.Invalidate(hole);

	EXPECT_EQ(cache.invalidate_event_count, 1);
	ASSERT_EQ(invalidated_signals.size(), 1);
	EXPECT_EQ(invalidated_signals.first(), hole);
	EXPECT_TRUE(cache.HasInvalidatedRanges(whole));

	const olive::TimeRangeList invalidated =
		cache.GetInvalidatedRanges(olive::rational(10));
	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(), hole);
}

TEST_F(RenderPlaybackCacheTest, InvalidateZeroLengthRangeIsIgnored)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);

	const olive::TimeRange whole(olive::rational(0), olive::rational(10));
	cache.Validate(whole);

	int invalidated_count = 0;
	QObject::connect(&cache, &olive::PlaybackCache::Invalidated,
					 [&invalidated_count](const olive::TimeRange &) {
						 invalidated_count++;
					 });

	// Zero-length invalidations are rejected with a warning
	cache.Invalidate(olive::TimeRange(olive::rational(5), olive::rational(5)));

	EXPECT_EQ(invalidated_count, 0);
	EXPECT_EQ(cache.invalidate_event_count, 0);
	EXPECT_TRUE(cache.GetValidatedRanges().contains(whole));
}

TEST_F(RenderPlaybackCacheTest, GetInvalidatedRangesClampsNegativeTimes)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);

	// Nothing is validated, so the whole (clamped) range is invalidated
	const olive::TimeRangeList invalidated = cache.GetInvalidatedRanges(
		olive::TimeRange(olive::rational(-5), olive::rational(10)));

	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first().in(), olive::rational(0));
	EXPECT_EQ(invalidated.first().out(), olive::rational(10));
}

TEST_F(RenderPlaybackCacheTest, PassthroughCoversInvalidatedRanges)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache source(node);
	TestPlaybackCache dest(node);

	const olive::TimeRange whole(olive::rational(0), olive::rational(10));
	source.Validate(whole);

	dest.SetPassthrough(&source);

	ASSERT_EQ(dest.GetPassthroughs().size(), 1);
	EXPECT_EQ(dest.GetPassthroughs().front().cache, source.GetUuid());
	EXPECT_EQ(dest.GetPassthroughs().front().in(), whole.in());
	EXPECT_EQ(dest.GetPassthroughs().front().out(), whole.out());

	// GetInvalidatedRanges honors passthroughs ...
	EXPECT_TRUE(dest.GetInvalidatedRanges(olive::rational(10)).isEmpty());
	// ... but HasInvalidatedRanges only looks at locally validated ranges
	EXPECT_TRUE(dest.HasInvalidatedRanges(whole));

	// Invalidating trims the passthrough too
	const olive::TimeRange hole(olive::rational(2), olive::rational(3));
	dest.Invalidate(hole);
	const olive::TimeRangeList invalidated =
		dest.GetInvalidatedRanges(olive::rational(10));
	ASSERT_EQ(invalidated.size(), 1);
	EXPECT_EQ(invalidated.first(), hole);
}

TEST_F(RenderPlaybackCacheTest, PassthroughChainsAcrossCaches)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache a(node);
	TestPlaybackCache b(node);
	TestPlaybackCache c(node);

	a.Validate(olive::TimeRange(olive::rational(0), olive::rational(10)));
	b.SetPassthrough(&a);
	c.SetPassthrough(&b);

	// c inherits b's passthrough of a
	ASSERT_EQ(c.GetPassthroughs().size(), 1);
	EXPECT_EQ(c.GetPassthroughs().front().cache, a.GetUuid());
	EXPECT_TRUE(c.GetInvalidatedRanges(olive::rational(10)).isEmpty());
}

TEST_F(RenderPlaybackCacheTest, RequestResignalAndClear)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);

	int requested_count = 0;
	olive::TimeRange last_requested;
	QObject::connect(&cache, &olive::PlaybackCache::Requested,
					 [&requested_count, &last_requested](
						 olive::ViewerOutput *context,
						 const olive::TimeRange &r) {
						 EXPECT_EQ(context, nullptr);
						 requested_count++;
						 last_requested = r;
					 });

	const olive::TimeRange first(olive::rational(0), olive::rational(5));
	const olive::TimeRange second(olive::rational(10), olive::rational(20));

	cache.Request(nullptr, first);
	EXPECT_EQ(requested_count, 1);
	EXPECT_EQ(last_requested, first);

	cache.Request(nullptr, second);
	EXPECT_EQ(requested_count, 2);

	// Both pending ranges are re-signaled
	cache.ResignalRequests();
	EXPECT_EQ(requested_count, 4);

	// Clearing one range leaves the other pending
	cache.ClearRequestRange(first);
	cache.ResignalRequests();
	EXPECT_EQ(requested_count, 5);
	EXPECT_EQ(last_requested, second);
}

TEST_F(RenderPlaybackCacheTest, InvalidateAllClearsEverything)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);
	TestPlaybackCache source(node);

	source.Validate(olive::TimeRange(olive::rational(0), olive::rational(10)));
	cache.Validate(olive::TimeRange(olive::rational(0), olive::rational(10)));
	cache.SetPassthrough(&source);

	QVector<olive::TimeRange> invalidated_signals;
	QObject::connect(&cache, &olive::PlaybackCache::Invalidated,
					 [&invalidated_signals](const olive::TimeRange &r) {
						 invalidated_signals.append(r);
					 });

	cache.InvalidateAll();

	EXPECT_FALSE(cache.HasValidatedRanges());
	EXPECT_TRUE(cache.GetPassthroughs().empty());
	ASSERT_EQ(invalidated_signals.size(), 1);
	EXPECT_EQ(invalidated_signals.first(),
			  olive::TimeRange(olive::rational(0), RATIONAL_MAX));
}

TEST_F(RenderPlaybackCacheTest, StatePersistsAcrossCaches)
{
	auto *node = AddNode<olive::MathNode>();
	const QUuid uuid = QUuid::createUuid();
	const QUuid source_uuid = QUuid::createUuid();

	const olive::TimeRange valid(olive::rational(5), olive::rational(15));
	const olive::TimeRange pass(olive::rational(20), olive::rational(30));

	{
		TestPlaybackCache cache(node);
		cache.SetUuid(uuid);
		cache.Validate(valid);

		TestPlaybackCache source(node);
		source.SetUuid(source_uuid);
		source.Validate(pass);
		cache.SetPassthrough(&source);

		EXPECT_GE(cache.save_state_event_count, 1);
	}

	const QString state_file =
		QDir(QDir(CacheRoot()).filePath(uuid.toString()))
			.filePath(QStringLiteral("state"));
	ASSERT_TRUE(QFileInfo::exists(state_file));

	TestPlaybackCache restored(node);
	restored.SetUuid(uuid);

	EXPECT_EQ(restored.load_state_event_count, 1);
	EXPECT_TRUE(restored.GetValidatedRanges().contains(valid));
	ASSERT_EQ(restored.GetPassthroughs().size(), 1);
	EXPECT_EQ(restored.GetPassthroughs().front().cache, source_uuid);
	EXPECT_EQ(restored.GetPassthroughs().front().in(), pass.in());
	EXPECT_EQ(restored.GetPassthroughs().front().out(), pass.out());
}

TEST_F(RenderPlaybackCacheTest, CacheDirectoryHelpers)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);

	const QUuid uuid = QUuid::createUuid();
	cache.SetUuid(uuid);

	EXPECT_EQ(olive::PlaybackCache::GetThisCacheDirectory(
				  QStringLiteral("/base/path"), uuid),
			  QDir(QDir(QStringLiteral("/base/path")).filePath(
				  uuid.toString())));

	EXPECT_EQ(cache.GetThisCacheDirectory(),
			  QDir(QDir(CacheRoot()).filePath(uuid.toString())));

	EXPECT_GT(olive::PlaybackCache::GetCacheIndicatorHeight(), 0);
}

TEST_F(RenderPlaybackCacheTest, DrawPaintsValidatedRangesGreen)
{
	auto *node = AddNode<olive::MathNode>();
	TestPlaybackCache cache(node);
	cache.Validate(olive::TimeRange(olive::rational(2), olive::rational(5)));

	QImage image(100, 10, QImage::Format_RGB32);
	image.fill(Qt::black);
	{
		QPainter painter(&image);
		cache.Draw(&painter, olive::rational(0), 10.0, QRect(0, 0, 100, 10));
	}

	// 10 px/s: validated seconds [2,5) cover pixels [20,50)
	EXPECT_EQ(image.pixelColor(30, 5), QColor(Qt::green));
	EXPECT_EQ(image.pixelColor(10, 5), QColor(Qt::red));
	EXPECT_EQ(image.pixelColor(60, 5), QColor(Qt::red));
}

TEST_F(RenderPlaybackCacheTest, AudioParametersRoundTrip)
{
	auto *node = AddNode<olive::MathNode>();
	olive::AudioPlaybackCache cache(node);

	const olive::AudioParams params(48000, olive::kChannelLayoutStereo,
									olive::SampleFormat::F32P);
	cache.SetParameters(params);
	EXPECT_TRUE(cache.GetParameters() == params);
	EXPECT_EQ(cache.GetParameters().sample_rate(), 48000);
	EXPECT_EQ(cache.GetParameters().channel_count(), 2);

	// Setting identical parameters again takes the no-op path
	cache.SetParameters(params);
	EXPECT_TRUE(cache.GetParameters() == params);

	const olive::AudioParams other(44100, olive::kChannelLayoutMono,
								   olive::SampleFormat::F32P);
	cache.SetParameters(other);
	EXPECT_EQ(cache.GetParameters().sample_rate(), 44100);
	EXPECT_EQ(cache.GetParameters().channel_count(), 1);
}

// NOTE: AudioPlaybackCache::WriteSilence() is intentionally not exercised: it
// calls WritePCM() with an empty SampleBuffer, which makes
// WritePartOfSampleBuffer() loop forever (the write cursor never advances when
// the buffer provides no bytes). See the bug notes in the test report.
TEST_F(RenderPlaybackCacheTest, WritePcmValidatesRangesAndWritesSegments)
{
	auto *node = AddNode<olive::MathNode>();
	olive::AudioPlaybackCache cache(node);

	const olive::AudioParams params(48000, olive::kChannelLayoutStereo,
									olive::SampleFormat::F32P);
	cache.SetParameters(params);

	// Two adjacent 0.1s ranges at 48kHz stereo float
	const olive::TimeRange r1(olive::rational(0), olive::rational(1, 10));
	const olive::TimeRange r2(olive::rational(1, 10), olive::rational(1, 5));
	const olive::TimeRange whole(olive::rational(0), olive::rational(1, 5));

	const qint64 total_bytes = params.time_to_bytes_per_channel(whole.length());
	const qint64 range_bytes = params.time_to_bytes_per_channel(r1.length());
	ASSERT_GT(total_bytes, 0);
	ASSERT_GT(range_bytes, 0);

	olive::SampleBuffer samples(params,
								size_t(params.time_to_samples(whole.length())));
	ASSERT_TRUE(samples.is_allocated());
	for (int ch = 0; ch < params.channel_count(); ch++) {
		for (int64_t i = 0; i < params.time_to_samples(whole.length()); i++) {
			samples.data(ch)[i] = 0.0001f * float(i) + float(ch);
		}
	}

	int validated_count = 0;
	QObject::connect(&cache, &olive::PlaybackCache::Validated,
					 [&validated_count](const olive::TimeRange &) {
						 validated_count++;
					 });

	cache.WritePCM(whole, { r1, r2 }, samples);

	// Adjacent ranges merge into one validated block
	EXPECT_EQ(validated_count, 2);
	EXPECT_TRUE(cache.GetValidatedRanges().contains(r1));
	EXPECT_TRUE(cache.GetValidatedRanges().contains(r2));
	EXPECT_FALSE(cache.HasInvalidatedRanges(whole));

	// One segment file per channel in this cache's directory
	const QDir cache_dir = cache.GetThisCacheDirectory();
	const QString seg_ch0 = cache_dir.filePath(QStringLiteral("0.0"));
	const QString seg_ch1 = cache_dir.filePath(QStringLiteral("0.1"));
	ASSERT_TRUE(QFileInfo::exists(seg_ch0));
	ASSERT_TRUE(QFileInfo::exists(seg_ch1));

	// The segment starts with exactly the PCM data that was written
	QFile f0(seg_ch0);
	ASSERT_TRUE(f0.open(QFile::ReadOnly));
	const QByteArray raw0 = f0.read(total_bytes);
	ASSERT_EQ(raw0.size(), total_bytes);
	EXPECT_EQ(std::memcmp(raw0.constData(), samples.data(0), size_t(total_bytes)),
			  0);

	QFile f1(seg_ch1);
	ASSERT_TRUE(f1.open(QFile::ReadOnly));
	const QByteArray raw1 = f1.read(total_bytes);
	ASSERT_EQ(raw1.size(), total_bytes);
	EXPECT_EQ(std::memcmp(raw1.constData(), samples.data(1), size_t(total_bytes)),
			  0);
}
