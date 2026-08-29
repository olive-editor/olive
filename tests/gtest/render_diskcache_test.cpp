#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <QVariant>

#include "codec/frame.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "render/diskmanager.h"
#include "render/framehashcache.h"

namespace
{

bool WriteFile(const QString &path, qint64 size)
{
	QFile file(path);
	if (!file.open(QFile::WriteOnly)) {
		return false;
	}
	file.write(QByteArray(static_cast<int>(size), 'x'));
	file.close();
	return true;
}

// Mirrors PlaybackCache::GetThisCacheDirectory + FrameHashCache::CachePathName:
// cached frames are stored as <cache_root>/<uuid>/<timestamp> with no extension.
QString ExpectedFrameFile(const QString &cache_root, const QUuid &uuid,
						  qint64 timestamp)
{
	return QDir(QDir(cache_root).filePath(uuid.toString()))
		.filePath(QString::number(timestamp));
}

olive::FramePtr MakeSolidFrame(int width, int height,
							   olive::core::PixelFormat format,
							   int channel_count,
							   const olive::core::Color &color)
{
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(
		olive::VideoParams(width, height, format, channel_count));
	frame->allocate();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			frame->set_pixel(x, y, color);
		}
	}
	return frame;
}

} // namespace

class RenderDiskCacheTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!temp_dir_.isValid()) {
			GTEST_FAIL() << "Failed to create temporary directory";
		}

		if (!olive::Core::instance()) {
			// Leaked intentionally: Core is process-wide and DiskCacheFolder
			// eviction calls Core::instance()->WarnCacheFull() (matches
			// viewer_display_repro_test).
			new olive::Core(olive::Core::CoreParams());
		}

		olive::DiskManager::CreateInstance();

		// Point the project cache at a folder alongside the (unsaved) project
		// file so every cache read/write stays inside the temporary directory.
		olive::ColorManager::SetUpDefaultConfig();
		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
		project_->set_filename(
			QDir(temp_dir_.path()).filePath(QStringLiteral("test.ove")));
		project_->SetCacheLocationSetting(
			olive::Project::kCacheStoreAlongsideProject);
	}

	void TearDown() override
	{
		project_.reset();
		olive::DiskManager::DestroyInstance();
	}

	QString CacheRoot() const
	{
		return QDir(temp_dir_.path()).filePath(QStringLiteral("cache"));
	}

	QString MakeSubDir(const QString &name) const
	{
		QDir root(temp_dir_.path());
		if (!root.mkpath(name)) {
			return QString();
		}
		return root.filePath(name);
	}

	QTemporaryDir temp_dir_;
	std::unique_ptr<olive::Project> project_;
};

TEST_F(RenderDiskCacheTest, ValidateTimestampCachesSingleFrame)
{
	olive::FrameHashCache cache(project_.get());
	cache.SetTimebase(olive::core::rational(1, 30));
	EXPECT_EQ(cache.GetTimebase(), olive::core::rational(1, 30));

	EXPECT_FALSE(cache.IsFrameCached(olive::core::rational(15, 30)));

	cache.ValidateTimestamp(15);

	// Validated range is [15/30, 16/30): in inclusive, out exclusive
	EXPECT_TRUE(cache.IsFrameCached(olive::core::rational(15, 30)));
	EXPECT_TRUE(cache.IsFrameCached(olive::core::rational(31, 60)));
	EXPECT_FALSE(cache.IsFrameCached(olive::core::rational(16, 30)));
	EXPECT_FALSE(cache.IsFrameCached(olive::core::rational(14, 30)));
}

TEST_F(RenderDiskCacheTest, ValidateTimeCachesOneTimebaseRange)
{
	olive::FrameHashCache cache(project_.get());
	cache.SetTimebase(olive::core::rational(1, 10));

	// Validates [0.5, 0.6)
	cache.ValidateTime(olive::core::rational(1, 2));

	EXPECT_TRUE(cache.IsFrameCached(olive::core::rational(1, 2)));
	EXPECT_TRUE(cache.IsFrameCached(olive::core::rational(59, 100)));
	EXPECT_FALSE(cache.IsFrameCached(olive::core::rational(6, 10)));
	EXPECT_FALSE(cache.IsFrameCached(olive::core::rational(4, 10)));
}

TEST_F(RenderDiskCacheTest, SaveAndLoadFloatFrameRoundTrips)
{
	const QString sub = MakeSubDir(QStringLiteral("exr_f32"));
	ASSERT_FALSE(sub.isEmpty());
	const QUuid uuid = QUuid::createUuid();

	olive::FramePtr frame =
		MakeSolidFrame(8, 6, olive::core::PixelFormat::F32,
					   olive::VideoParams::kRGBAChannelCount,
					   olive::core::Color(0.25f, 0.5f, 0.75f, 1.0f));

	ASSERT_TRUE(olive::FrameHashCache::SaveCacheFrame(sub, uuid, 12345, frame));

	const QString fn = ExpectedFrameFile(sub, uuid, 12345);
	ASSERT_TRUE(QFileInfo::exists(fn));

	olive::FramePtr loaded =
		olive::FrameHashCache::LoadCacheFrame(sub, uuid, 12345);
	ASSERT_NE(loaded, nullptr);
	EXPECT_EQ(loaded->width(), 8);
	EXPECT_EQ(loaded->height(), 6);
	EXPECT_EQ(loaded->format(), olive::core::PixelFormat::F32);
	EXPECT_EQ(loaded->channel_count(), int(olive::VideoParams::kRGBAChannelCount));

	// EXR storage uses lossy DWAA compression; a solid color survives it well
	const olive::core::Color px = loaded->get_pixel(4, 3);
	EXPECT_NEAR(px.red(), 0.25f, 0.05);
	EXPECT_NEAR(px.green(), 0.5f, 0.05);
	EXPECT_NEAR(px.blue(), 0.75f, 0.05);
}

TEST_F(RenderDiskCacheTest, SaveAndLoadHalfFloatRgbFrameRoundTrips)
{
	const QString sub = MakeSubDir(QStringLiteral("exr_f16"));
	ASSERT_FALSE(sub.isEmpty());
	const QUuid uuid = QUuid::createUuid();

	olive::FramePtr frame =
		MakeSolidFrame(8, 6, olive::core::PixelFormat::F16,
					   olive::VideoParams::kRGBChannelCount,
					   olive::core::Color(0.5f, 0.5f, 0.5f, 1.0f));

	ASSERT_TRUE(olive::FrameHashCache::SaveCacheFrame(sub, uuid, 7, frame));
	ASSERT_TRUE(QFileInfo::exists(ExpectedFrameFile(sub, uuid, 7)));

	// RGB-only frames are stored without an alpha channel
	olive::FramePtr loaded = olive::FrameHashCache::LoadCacheFrame(sub, uuid, 7);
	ASSERT_NE(loaded, nullptr);
	EXPECT_EQ(loaded->width(), 8);
	EXPECT_EQ(loaded->height(), 6);
	EXPECT_EQ(loaded->format(), olive::core::PixelFormat::F16);
	EXPECT_EQ(loaded->channel_count(), int(olive::VideoParams::kRGBChannelCount));

	const olive::core::Color px = loaded->get_pixel(2, 2);
	EXPECT_NEAR(px.red(), 0.5f, 0.05);
}

TEST_F(RenderDiskCacheTest, SaveAndLoadU8FrameRoundTripsThroughJpeg)
{
	const QString sub = MakeSubDir(QStringLiteral("jpg_u8"));
	ASSERT_FALSE(sub.isEmpty());
	const QUuid uuid = QUuid::createUuid();

	olive::FramePtr frame =
		MakeSolidFrame(16, 16, olive::core::PixelFormat::U8,
					   olive::VideoParams::kRGBAChannelCount,
					   olive::core::Color(0.5f, 0.5f, 0.5f, 1.0f));

	ASSERT_TRUE(olive::FrameHashCache::SaveCacheFrame(sub, uuid, 3, frame));
	ASSERT_TRUE(QFileInfo::exists(ExpectedFrameFile(sub, uuid, 3)));

	// Integer formats fall back to JPEG; the loader hardcodes 4 channels
	olive::FramePtr loaded = olive::FrameHashCache::LoadCacheFrame(sub, uuid, 3);
	ASSERT_NE(loaded, nullptr);
	EXPECT_EQ(loaded->width(), 16);
	EXPECT_EQ(loaded->height(), 16);
	EXPECT_EQ(loaded->format(), olive::core::PixelFormat::U8);
	EXPECT_EQ(loaded->channel_count(), 4);

	// Gray is unaffected by channel order and survives JPEG nearly intact
	const olive::core::Color px = loaded->get_pixel(8, 8);
	EXPECT_NEAR(px.red(), 0.5f, 0.05);
}

TEST_F(RenderDiskCacheTest, SaveAndLoadWithEmptyCachePathFail)
{
	olive::FramePtr frame =
		MakeSolidFrame(4, 4, olive::core::PixelFormat::F32,
					   olive::VideoParams::kRGBAChannelCount,
					   olive::core::Color(1.0f, 1.0f, 1.0f, 1.0f));

	EXPECT_FALSE(olive::FrameHashCache::SaveCacheFrame(
		QString(), QUuid::createUuid(), 1, frame));
	EXPECT_EQ(olive::FrameHashCache::LoadCacheFrame(
				  QString(), QUuid::createUuid(), 1),
			  nullptr);
}

TEST_F(RenderDiskCacheTest, LoadOfMissingFileReturnsNull)
{
	const QString sub = MakeSubDir(QStringLiteral("missing"));
	ASSERT_FALSE(sub.isEmpty());

	EXPECT_EQ(olive::FrameHashCache::LoadCacheFrame(sub, QUuid::createUuid(),
													555),
			  nullptr);
}

TEST_F(RenderDiskCacheTest, LoadingCorruptFileReturnsNullAndDeletesIt)
{
	const QString sub = MakeSubDir(QStringLiteral("corrupt"));
	ASSERT_FALSE(sub.isEmpty());
	const QUuid uuid = QUuid::createUuid();

	const QString fn = ExpectedFrameFile(sub, uuid, 999);
	ASSERT_TRUE(QDir().mkpath(QFileInfo(fn).absolutePath()));
	ASSERT_TRUE(WriteFile(fn, 64)); // neither EXR nor JPEG

	// The corrupt frame must be registered for the disk manager to delete it
	olive::DiskManager::instance()->CreatedFile(sub, fn);

	EXPECT_EQ(olive::FrameHashCache::LoadCacheFrame(sub, uuid, 999), nullptr);
	EXPECT_FALSE(QFileInfo::exists(fn));
}

TEST_F(RenderDiskCacheTest, SavingUnsupportedPixelFormatFails)
{
	const QString sub = MakeSubDir(QStringLiteral("unsupported"));
	ASSERT_FALSE(sub.isEmpty());

	// U10 is a packed format with no EXR/QImage writer in FrameHashCache
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(
		olive::VideoParams(8, 8, olive::core::PixelFormat::U10,
						   olive::VideoParams::kRGBAChannelCount));
	frame->allocate();

	const QString fn = QDir(sub).filePath(QStringLiteral("u10_frame"));
	EXPECT_FALSE(olive::FrameHashCache::SaveCacheFrame(fn, frame));
	EXPECT_FALSE(QFileInfo::exists(fn));
}

TEST_F(RenderDiskCacheTest, SaveCacheFrameRegistersFolderWithDiskManager)
{
	const QString sub = MakeSubDir(QStringLiteral("registered"));
	ASSERT_FALSE(sub.isEmpty());
	const QUuid uuid = QUuid::createUuid();

	olive::FramePtr frame =
		MakeSolidFrame(4, 4, olive::core::PixelFormat::F32,
					   olive::VideoParams::kRGBAChannelCount,
					   olive::core::Color(0.0f, 0.0f, 0.0f, 1.0f));

	olive::DiskManager *dm = olive::DiskManager::instance();
	const int folder_count_before = dm->GetOpenFolders().size();

	ASSERT_TRUE(olive::FrameHashCache::SaveCacheFrame(sub, uuid, 42, frame));

	EXPECT_EQ(dm->GetOpenFolders().size(), folder_count_before + 1);

	// Registration means the folder now tracks the file for deletion
	olive::DiskCacheFolder *folder = dm->GetOpenFolder(sub);
	ASSERT_NE(folder, nullptr);
	EXPECT_TRUE(folder->DeleteSpecificFile(ExpectedFrameFile(sub, uuid, 42)));
}

TEST_F(RenderDiskCacheTest, GetValidCacheFilenameRequiresValidatedFrame)
{
	olive::FrameHashCache cache(project_.get());
	cache.SetTimebase(olive::core::rational(1, 30));

	const olive::core::rational t(15, 30);
	EXPECT_TRUE(cache.GetValidCacheFilename(t).isEmpty());

	cache.ValidateTimestamp(15);

	const QString fn = cache.GetValidCacheFilename(t);
	EXPECT_EQ(fn, ExpectedFrameFile(CacheRoot(), cache.GetUuid(), 15));
}

TEST_F(RenderDiskCacheTest, DeletingFrameThroughDiskManagerInvalidatesRange)
{
	olive::FrameHashCache cache(project_.get());
	cache.SetTimebase(olive::core::rational(1, 30));

	olive::FramePtr frame =
		MakeSolidFrame(4, 4, olive::core::PixelFormat::F32,
					   olive::VideoParams::kRGBAChannelCount,
					   olive::core::Color(1.0f, 0.0f, 0.0f, 1.0f));

	// Instance overloads resolve the cache dir/uuid from the parent project
	ASSERT_TRUE(cache.SaveCacheFrame(15, frame));

	const QString fn = ExpectedFrameFile(CacheRoot(), cache.GetUuid(), 15);
	ASSERT_TRUE(QFileInfo::exists(fn));
	ASSERT_NE(cache.LoadCacheFrame(15), nullptr);

	const olive::core::rational t(15, 30);
	cache.ValidateTimestamp(15);
	ASSERT_TRUE(cache.IsFrameCached(t));

	// Deletion must propagate through DiskManager::DeletedFrame into HashDeleted
	olive::DiskManager::instance()->DeleteSpecificFile(fn);

	EXPECT_FALSE(QFileInfo::exists(fn));
	EXPECT_FALSE(cache.IsFrameCached(t));
}

TEST_F(RenderDiskCacheTest, DiskDeletedSignalFromForeignCacheDoesNotInvalidate)
{
	olive::FrameHashCache cache(project_.get());
	cache.SetTimebase(olive::core::rational(1, 30));
	cache.ValidateTimestamp(15);

	const olive::core::rational t(15, 30);
	ASSERT_TRUE(cache.IsFrameCached(t));

	// Different cache directory: must be ignored
	emit olive::DiskManager::instance()
		->DeletedFrame(QStringLiteral("/some/other/cache"),
					   QStringLiteral("/some/other/cache/15"));
	EXPECT_TRUE(cache.IsFrameCached(t));

	// Same directory but a different cache UUID: must be ignored
	emit olive::DiskManager::instance()
		->DeletedFrame(CacheRoot(),
					   ExpectedFrameFile(CacheRoot(), QUuid::createUuid(), 15));
	EXPECT_TRUE(cache.IsFrameCached(t));
}

TEST_F(RenderDiskCacheTest, InvalidateProjectSignalClearsValidatedRanges)
{
	olive::FrameHashCache cache(project_.get());
	cache.SetTimebase(olive::core::rational(1, 30));

	const olive::core::rational t(15, 30);
	cache.ValidateTimestamp(15);
	ASSERT_TRUE(cache.IsFrameCached(t));

	// An unrelated project must not invalidate this cache
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project other;
	emit olive::DiskManager::instance()->InvalidateProject(&other);
	EXPECT_TRUE(cache.IsFrameCached(t));

	emit olive::DiskManager::instance()->InvalidateProject(project_.get());
	EXPECT_FALSE(cache.IsFrameCached(t));
}

TEST_F(RenderDiskCacheTest, ValidatedStatePersistsAcrossCaches)
{
	const QUuid uuid = QUuid::createUuid();

	{
		olive::FrameHashCache cache(project_.get());
		cache.SetUuid(uuid);
		cache.SetTimebase(olive::core::rational(1, 30));
		cache.ValidateTimestamp(15);
	}

	const QString state_file =
		QDir(QDir(CacheRoot()).filePath(uuid.toString()))
			.filePath(QStringLiteral("state"));
	ASSERT_TRUE(QFileInfo::exists(state_file));

	// SetUuid triggers LoadState, restoring timebase and validated ranges
	olive::FrameHashCache restored(project_.get());
	restored.SetUuid(uuid);

	EXPECT_EQ(restored.GetTimebase(), olive::core::rational(1, 30));
	EXPECT_TRUE(restored.IsFrameCached(olive::core::rational(15, 30)));
	EXPECT_FALSE(restored.IsFrameCached(olive::core::rational(16, 30)));
}

TEST_F(RenderDiskCacheTest, PassthroughProvidesFilenameForUnvalidatedFrame)
{
	const olive::core::rational tb(1, 30);

	olive::FrameHashCache source(project_.get());
	source.SetTimebase(tb);
	source.ValidateTimestamp(15);

	olive::FrameHashCache dest(project_.get());
	dest.SetPassthrough(&source);

	// SetPassthrough adopts the source cache's timebase
	EXPECT_EQ(dest.GetTimebase(), tb);

	// The frame is not validated locally but the passthrough covers it
	const olive::core::rational t(15, 30);
	EXPECT_FALSE(dest.IsFrameCached(t));
	EXPECT_EQ(dest.GetValidCacheFilename(t),
			  ExpectedFrameFile(CacheRoot(), source.GetUuid(), 15));
}

TEST_F(RenderDiskCacheTest, ThumbnailCacheUsesFixedTimebase)
{
	olive::ThumbnailCache cache(project_.get());
	EXPECT_EQ(cache.GetTimebase(), olive::core::rational(1, 10));
}

TEST_F(RenderDiskCacheTest, FolderDefaultsToTwentyGbLimit)
{
	const QString sub = MakeSubDir(QStringLiteral("folder_defaults"));
	ASSERT_FALSE(sub.isEmpty());

	olive::DiskCacheFolder folder(sub);
	EXPECT_EQ(folder.GetPath(), sub);
	EXPECT_EQ(folder.GetLimit(), 21474836480LL); // 20 GB
	EXPECT_FALSE(folder.GetClearOnClose());
}

TEST_F(RenderDiskCacheTest, CreatedFileCanBeDeletedSpecifically)
{
	const QString sub = MakeSubDir(QStringLiteral("delete_specific"));
	ASSERT_FALSE(sub.isEmpty());

	olive::DiskCacheFolder folder(sub);

	const QString fn = QDir(sub).filePath(QStringLiteral("frame1"));
	ASSERT_TRUE(WriteFile(fn, 128));
	folder.CreatedFile(fn);

	QSignalSpy spy(&folder, &olive::DiskCacheFolder::DeletedFrame);

	EXPECT_TRUE(folder.DeleteSpecificFile(fn));
	EXPECT_FALSE(QFileInfo::exists(fn));

	ASSERT_EQ(spy.count(), 1);
	const QList<QVariant> args = spy.takeFirst();
	EXPECT_EQ(args.at(0).toString(), sub);
	EXPECT_EQ(args.at(1).toString(), fn);

	// A second deletion attempt fails, as does deleting an unknown file
	EXPECT_FALSE(folder.DeleteSpecificFile(fn));
	EXPECT_FALSE(folder.DeleteSpecificFile(
		QDir(sub).filePath(QStringLiteral("never_registered"))));
}

TEST_F(RenderDiskCacheTest, ClearCacheRemovesAllRegisteredFiles)
{
	const QString sub = MakeSubDir(QStringLiteral("clear_cache"));
	ASSERT_FALSE(sub.isEmpty());

	olive::DiskCacheFolder folder(sub);

	const QString f1 = QDir(sub).filePath(QStringLiteral("a"));
	const QString f2 = QDir(sub).filePath(QStringLiteral("b"));
	ASSERT_TRUE(WriteFile(f1, 32));
	ASSERT_TRUE(WriteFile(f2, 32));
	folder.CreatedFile(f1);
	folder.CreatedFile(f2);

	QSignalSpy spy(&folder, &olive::DiskCacheFolder::DeletedFrame);

	EXPECT_TRUE(folder.ClearCache());
	EXPECT_FALSE(QFileInfo::exists(f1));
	EXPECT_FALSE(QFileInfo::exists(f2));
	EXPECT_EQ(spy.count(), 2);
}

TEST_F(RenderDiskCacheTest, ClearCacheToleratesExternallyRemovedFiles)
{
	const QString sub = MakeSubDir(QStringLiteral("clear_missing"));
	ASSERT_FALSE(sub.isEmpty());

	olive::DiskCacheFolder folder(sub);

	const QString fn = QDir(sub).filePath(QStringLiteral("gone"));
	ASSERT_TRUE(WriteFile(fn, 32));
	folder.CreatedFile(fn);

	ASSERT_TRUE(QFile::remove(fn));

	// Already-missing files count as successfully cleared
	EXPECT_TRUE(folder.ClearCache());
}

TEST_F(RenderDiskCacheTest, FolderStatePersistsAcrossInstances)
{
	const QString sub = MakeSubDir(QStringLiteral("persist"));
	ASSERT_FALSE(sub.isEmpty());

	const QString fn = QDir(sub).filePath(QStringLiteral("persisted_frame"));
	ASSERT_TRUE(WriteFile(fn, 64));

	{
		olive::DiskCacheFolder folder(sub);
		folder.SetLimit(12345);
		folder.CreatedFile(fn);
		// Destruction writes the index file into the cache folder
	}

	{
		olive::DiskCacheFolder reopened(sub);
		EXPECT_EQ(reopened.GetLimit(), 12345);
		EXPECT_FALSE(reopened.GetClearOnClose());

		// The persisted entry is only known if the index was reloaded
		EXPECT_TRUE(reopened.DeleteSpecificFile(fn));
		EXPECT_FALSE(QFileInfo::exists(fn));
	}
}

TEST_F(RenderDiskCacheTest, ExceedingLimitEvictsLeastRecentlyUsedFile)
{
	const QString sub = MakeSubDir(QStringLiteral("eviction"));
	ASSERT_FALSE(sub.isEmpty());

	olive::DiskCacheFolder folder(sub);
	folder.SetLimit(250);
	EXPECT_EQ(folder.GetLimit(), 250);

	// Names are ordered so that even identical timestamps evict "aaa_evict"
	const QString keep = QDir(sub).filePath(QStringLiteral("zzz_keep"));
	const QString evict = QDir(sub).filePath(QStringLiteral("aaa_evict"));
	const QString newest = QDir(sub).filePath(QStringLiteral("bbb_newest"));

	ASSERT_TRUE(WriteFile(keep, 100));
	folder.CreatedFile(keep);

	QThread::msleep(20);

	ASSERT_TRUE(WriteFile(evict, 100));
	folder.CreatedFile(evict);

	// Both files fit within the limit
	ASSERT_TRUE(QFileInfo::exists(keep));
	ASSERT_TRUE(QFileInfo::exists(evict));

	// Unknown filenames are ignored by Accessed
	folder.Accessed(QDir(sub).filePath(QStringLiteral("unknown")));

	QThread::msleep(20);

	// "keep" becomes the most recently used file
	folder.Accessed(keep);

	QSignalSpy spy(&folder, &olive::DiskCacheFolder::DeletedFrame);

	ASSERT_TRUE(WriteFile(newest, 100));
	folder.CreatedFile(newest); // 300 > 250, one eviction required

	EXPECT_TRUE(QFileInfo::exists(keep));
	EXPECT_FALSE(QFileInfo::exists(evict));
	EXPECT_TRUE(QFileInfo::exists(newest));

	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().at(1).toString(), evict);
}

TEST_F(RenderDiskCacheTest, ClearOnCloseDeletesFilesWhenFolderCloses)
{
	const QString sub = MakeSubDir(QStringLiteral("clear_on_close"));
	ASSERT_FALSE(sub.isEmpty());

	const QString fn = QDir(sub).filePath(QStringLiteral("closing_frame"));
	ASSERT_TRUE(WriteFile(fn, 32));

	{
		olive::DiskCacheFolder folder(sub);
		folder.SetClearOnClose(true);
		EXPECT_TRUE(folder.GetClearOnClose());
		folder.CreatedFile(fn);
	}

	EXPECT_FALSE(QFileInfo::exists(fn));
}

TEST_F(RenderDiskCacheTest, DiskManagerOpenFolderDeduplicates)
{
	olive::DiskManager *dm = olive::DiskManager::instance();
	ASSERT_NE(dm, nullptr);

	const QString sub = MakeSubDir(QStringLiteral("dedupe"));
	ASSERT_FALSE(sub.isEmpty());

	const int folder_count_before = dm->GetOpenFolders().size();

	olive::DiskCacheFolder *first = dm->GetOpenFolder(sub);
	olive::DiskCacheFolder *second = dm->GetOpenFolder(sub);

	ASSERT_NE(first, nullptr);
	EXPECT_EQ(first, second);
	EXPECT_EQ(first->GetPath(), sub);
	EXPECT_EQ(dm->GetOpenFolders().size(), folder_count_before + 1);

	// An empty path resolves to the default cache folder
	EXPECT_EQ(dm->GetOpenFolder(QString()), dm->GetDefaultCacheFolder());
	EXPECT_FALSE(dm->GetDefaultCachePath().isEmpty());
}

TEST_F(RenderDiskCacheTest, DiskManagerClearDiskCacheRemovesFiles)
{
	olive::DiskManager *dm = olive::DiskManager::instance();
	ASSERT_NE(dm, nullptr);

	const QString sub = MakeSubDir(QStringLiteral("managed_clear"));
	ASSERT_FALSE(sub.isEmpty());

	const QString fn = QDir(sub).filePath(QStringLiteral("managed_frame"));
	ASSERT_TRUE(WriteFile(fn, 32));
	dm->CreatedFile(sub, fn);
	ASSERT_TRUE(QFileInfo::exists(fn));

	EXPECT_TRUE(dm->ClearDiskCache(sub));
	EXPECT_FALSE(QFileInfo::exists(fn));
}
