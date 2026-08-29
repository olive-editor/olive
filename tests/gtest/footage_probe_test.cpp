#include <gtest/gtest.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVariant>
#include <QWidget>

#include "codec/decoder.h"
#include "common/filefunctions.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/globals.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "node/project/footage/footagedescription.h"
#include "render/diskmanager.h"
#include "render/job/footagejob.h"
#include "render/loopmode.h"
#include "render/texture.h"

namespace
{

QString DemoVideoPath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/demo.mp4"));
}

QString TestImagePath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/img.png"));
}

// Mirrors the cache path expression used by Footage::Reprobe()
QString MetadataCacheFileFor(const QString &media_path)
{
	return QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
		.filePath(olive::FileFunctions::GetUniqueFileIdentifier(media_path));
}

} // namespace

TEST(FootageProbe, FFmpegProbeOfDemoMp4ReportsExpectedStreams)
{
	const QString path = DemoVideoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::DecoderPtr decoder =
		olive::Decoder::CreateFromID(QStringLiteral("ffmpeg"));
	ASSERT_TRUE(decoder);

	const olive::FootageDescription desc = decoder->Probe(path, nullptr);
	ASSERT_TRUE(desc.IsValid());
	EXPECT_EQ(desc.decoder(), QStringLiteral("ffmpeg"));

	// The file holds video + audio + a timecode data track. The data track is
	// counted in the total but not exposed as a usable stream.
	ASSERT_EQ(desc.GetVideoStreams().size(), 1);
	ASSERT_EQ(desc.GetAudioStreams().size(), 1);
	EXPECT_EQ(desc.GetSubtitleStreams().size(), 0);
	EXPECT_EQ(desc.GetStreamCount(), 3);

	const olive::VideoParams &video = desc.GetVideoStreams().first();
	EXPECT_EQ(video.stream_index(), 0);
	EXPECT_EQ(video.width(), 1920);
	EXPECT_EQ(video.height(), 1080);
	EXPECT_EQ(video.video_type(), olive::VideoParams::kVideoTypeVideo);
	EXPECT_EQ(video.interlacing(), olive::VideoParams::kInterlaceNone);
	EXPECT_EQ(video.pixel_aspect_ratio(), olive::rational(1, 1));
	EXPECT_EQ(video.frame_rate(), olive::rational(25));
	EXPECT_EQ(video.time_base(), olive::rational(1, 12800));
	EXPECT_EQ(video.duration(), 217600); // 17 seconds at 1/12800
	EXPECT_NE(video.format(), olive::core::PixelFormat::INVALID);
	EXPECT_GT(video.channel_count(), 0);

	const olive::core::AudioParams &audio = desc.GetAudioStreams().first();
	EXPECT_EQ(audio.stream_index(), 1);
	EXPECT_EQ(audio.sample_rate(), 48000);
	EXPECT_EQ(audio.channel_count(), 2);
	EXPECT_EQ(audio.time_base(), olive::rational(1, 48000));
	EXPECT_EQ(audio.duration(), 816000); // 17 seconds at 1/48000

	// The file's timecode track starts at 01:00:00:00
	ASSERT_TRUE(desc.HasSourceStartTime());
	EXPECT_EQ(desc.source_start_time(), olive::rational(3600));
	EXPECT_EQ(desc.source_start_time_source(), QStringLiteral("timecode"));
}

TEST(FootageProbe, ProbeOfUnprobeableFileYieldsInvalidDescription)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString path =
		QDir(dir.path()).filePath(QStringLiteral("not_media.txt"));
	{
		QFile file(path);
		ASSERT_TRUE(file.open(QFile::WriteOnly));
		file.write("this is not a media file");
	}

	for (const olive::DecoderPtr &decoder :
		 olive::Decoder::ReceiveListOfAllDecoders()) {
		EXPECT_FALSE(decoder->Probe(path, nullptr).IsValid())
			<< decoder->id().toStdString();
	}
}

class FootageProbeTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!olive::Core::instance()) {
			// Leaked intentionally: Core is process-wide (matches footage_test)
			new olive::Core(olive::Core::CoreParams());
		}

		// Footage::Value() resolves Project::cache_path(), which goes through
		// the DiskManager singleton
		created_disk_manager_ = (olive::DiskManager::instance() == nullptr);
		if (created_disk_manager_) {
			olive::DiskManager::CreateInstance();
		}

		// Sandbox the footage metadata cache so real probes write into the
		// temp dir instead of the user's cache
		old_cache_home_ = qgetenv("XDG_CACHE_HOME");
		had_cache_home_ = qEnvironmentVariableIsSet("XDG_CACHE_HOME");
		qputenv("XDG_CACHE_HOME",
				QDir(temp_dir_.path()).filePath(QStringLiteral("xdg")).toUtf8());
		QDir().mkpath(
			QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

		olive::ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	void TearDown() override
	{
		project_.reset();
		if (created_disk_manager_) {
			olive::DiskManager::DestroyInstance();
		}
		if (had_cache_home_) {
			qputenv("XDG_CACHE_HOME", old_cache_home_);
		} else {
			qunsetenv("XDG_CACHE_HOME");
		}
	}

	// Constructs a Footage pointing at path; the constructor's set_filename()
	// call probes the file synchronously before the node joins the graph
	olive::Footage *AddProbedFootage(const QString &path)
	{
		auto *footage = new olive::Footage(path);
		footage->setParent(project_.get());
		return footage;
	}

	QTemporaryDir temp_dir_;
	QByteArray old_cache_home_;
	bool had_cache_home_ = false;
	bool created_disk_manager_ = false;
	std::unique_ptr<olive::Project> project_;
};

TEST_F(FootageProbeTest, ProbingDemoMp4PopulatesFootageState)
{
	const QString path = DemoVideoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::Footage *footage = AddProbedFootage(path);

	EXPECT_TRUE(footage->IsValid());
	EXPECT_EQ(footage->decoder(), QStringLiteral("ffmpeg"));
	EXPECT_EQ(footage->timestamp(),
			  QFileInfo(path).lastModified().toMSecsSinceEpoch());

	// Video + audio streams are usable; the timecode data track only shows up
	// in the total stream count
	EXPECT_EQ(footage->GetTotalStreamCount(), 3);
	EXPECT_EQ(footage->GetVideoStreamCount(), 1);
	EXPECT_EQ(footage->GetAudioStreamCount(), 1);
	EXPECT_EQ(footage->GetSubtitleStreamCount(), 0);

	EXPECT_EQ(footage->GetStreamIndex(olive::Track::kVideo, 0), 0);
	EXPECT_EQ(footage->GetStreamIndex(olive::Track::kAudio, 0), 1);
	EXPECT_EQ(footage->GetReferenceFromRealIndex(0),
			  olive::Track::Reference(olive::Track::kVideo, 0));
	EXPECT_EQ(footage->GetReferenceFromRealIndex(1),
			  olive::Track::Reference(olive::Track::kAudio, 0));
	EXPECT_EQ(footage->GetReferenceFromRealIndex(2).type(),
			  olive::Track::kNone);

	EXPECT_EQ(footage->GetConnectedTextureOutput(),
			  static_cast<olive::Node *>(footage));
	EXPECT_EQ(footage->GetConnectedSampleOutput(),
			  static_cast<olive::Node *>(footage));

	const olive::VideoParams video = footage->GetVideoParams(0);
	ASSERT_TRUE(video.is_valid());
	EXPECT_EQ(video.stream_index(), 0);
	EXPECT_EQ(video.width(), 1920);
	EXPECT_EQ(video.height(), 1080);
	EXPECT_EQ(video.video_type(), olive::VideoParams::kVideoTypeVideo);
	EXPECT_EQ(video.frame_rate(), olive::rational(25));
	EXPECT_EQ(video.time_base(), olive::rational(1, 12800));
	EXPECT_EQ(video.duration(), 217600);
	EXPECT_EQ(video.color_range(), olive::VideoParams::kColorRangeLimited);
	EXPECT_TRUE(video.enabled());
	// The FFmpeg probe leaves colorspace unset so the project default applies
	EXPECT_TRUE(video.colorspace().isEmpty());

	const olive::core::AudioParams audio = footage->GetAudioParams(0);
	ASSERT_TRUE(audio.is_valid());
	EXPECT_EQ(audio.stream_index(), 1);
	EXPECT_EQ(audio.sample_rate(), 48000);
	EXPECT_EQ(audio.channel_count(), 2);
	EXPECT_EQ(audio.duration(), 816000);
	EXPECT_TRUE(audio.enabled());
}

TEST_F(FootageProbeTest, ProbingDemoMp4SetsLengthsAndSourceStartTime)
{
	const QString path = DemoVideoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::Footage *footage = AddProbedFootage(path);
	footage->VerifyLength();

	// Both streams describe 17 seconds of media
	EXPECT_EQ(footage->GetVideoLength(), olive::rational(17));
	EXPECT_EQ(footage->GetAudioLength(), olive::rational(17));
	EXPECT_EQ(footage->GetLength(), olive::rational(17));

	// The embedded 01:00:00:00 timecode becomes the source start time
	ASSERT_TRUE(footage->HasSourceStartTime());
	EXPECT_EQ(footage->source_start_time(), olive::rational(3600));
	EXPECT_EQ(footage->source_start_time_source(), QStringLiteral("timecode"));
}

TEST_F(FootageProbeTest, ProbingPngImageProducesSingleStillStream)
{
	const QString path = TestImagePath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::Footage *footage = AddProbedFootage(path);

	EXPECT_TRUE(footage->IsValid());
	// Still images are handled by the OIIO decoder, which probes before FFmpeg
	EXPECT_EQ(footage->decoder(), QStringLiteral("oiio"));

	EXPECT_EQ(footage->GetTotalStreamCount(), 1);
	EXPECT_EQ(footage->GetVideoStreamCount(), 1);
	EXPECT_EQ(footage->GetAudioStreamCount(), 0);
	EXPECT_EQ(footage->GetSubtitleStreamCount(), 0);

	const olive::VideoParams still = footage->GetVideoParams(0);
	ASSERT_TRUE(still.is_valid());
	EXPECT_EQ(still.stream_index(), 0);
	EXPECT_EQ(still.width(), 1920);
	EXPECT_EQ(still.height(), 1080);
	EXPECT_EQ(still.video_type(), olive::VideoParams::kVideoTypeStill);
	EXPECT_EQ(still.channel_count(), 4);
	EXPECT_EQ(still.format(), olive::core::PixelFormat::U8);
	EXPECT_TRUE(still.premultiplied_alpha());
	EXPECT_TRUE(still.enabled());
	EXPECT_TRUE(still.colorspace().isEmpty());

	// Stills have no duration and no source start time
	footage->VerifyLength();
	EXPECT_EQ(footage->GetVideoLength(), olive::rational(0));
	EXPECT_EQ(footage->GetLength(), olive::rational(0));
	EXPECT_FALSE(footage->HasSourceStartTime());

	EXPECT_EQ(footage->GetConnectedTextureOutput(),
			  static_cast<olive::Node *>(footage));
	EXPECT_EQ(footage->GetConnectedSampleOutput(), nullptr);
}

TEST_F(FootageProbeTest, ProbedFootageValuePushesRealStreamJobs)
{
	const QString path = DemoVideoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::Footage *footage = AddProbedFootage(path);
	footage->VerifyLength();

	// The colorspace fallback reads the project default, and the audio cache
	// path comes from the project's cache settings
	project_->SetDefaultInputColorSpace(QStringLiteral("ProbeInputSpace"));
	project_->SetCacheLocationSetting(olive::Project::kCacheCustomPath);
	const QString cache_path =
		QDir(temp_dir_.path()).filePath(QStringLiteral("cache"));
	project_->SetCustomCachePath(cache_path);

	olive::NodeValueRow row;
	row.insert(olive::Footage::kFilenameInput,
			   olive::NodeValue(olive::NodeValue::kFile, path));

	olive::VideoParams vparams(64, 64, olive::rational(1, 24),
							   olive::core::PixelFormat::U8, 4);
	const olive::NodeGlobals globals(vparams, olive::core::AudioParams(),
									 olive::rational(0),
									 olive::LoopMode::kLoopModeOff);

	olive::NodeValueTable table;
	footage->Value(row, globals, &table);

	// Length, one texture job for the video stream, one sample job for the
	// audio stream; the timecode data track produces no job
	ASSERT_EQ(table.Count(), 3);

	const olive::NodeValue length =
		table.Get(olive::NodeValue::kRational, QStringLiteral("length"));
	ASSERT_EQ(length.type(), olive::NodeValue::kRational);
	EXPECT_EQ(length.toRational(), olive::rational(17));

	const olive::TexturePtr texture =
		table.Get(olive::NodeValue::kTexture, QStringLiteral("v:0"))
			.toTexture();
	ASSERT_NE(texture, nullptr);
	EXPECT_EQ(texture->params().width(), 1920);
	EXPECT_EQ(texture->params().height(), 1080);
	// The probed stream has no colorspace, so the project default is used
	EXPECT_EQ(texture->params().colorspace(),
			  QStringLiteral("ProbeInputSpace"));

	const auto *video_job =
		static_cast<const olive::FootageJob *>(texture->job());
	ASSERT_NE(video_job, nullptr);
	EXPECT_EQ(video_job->decoder(), QStringLiteral("ffmpeg"));
	EXPECT_EQ(video_job->filename(), path);
	EXPECT_EQ(video_job->type(), olive::Track::kVideo);
	EXPECT_EQ(video_job->length(), olive::rational(17));

	const olive::FootageJob audio_job =
		table.Get(olive::NodeValue::kSamples, QStringLiteral("a:0"))
			.data()
			.value<olive::FootageJob>();
	EXPECT_EQ(audio_job.decoder(), QStringLiteral("ffmpeg"));
	EXPECT_EQ(audio_job.filename(), path);
	EXPECT_EQ(audio_job.type(), olive::Track::kAudio);
	EXPECT_EQ(audio_job.audio_params().sample_rate(), 48000);
	EXPECT_EQ(audio_job.length(), olive::rational(17));
	EXPECT_EQ(audio_job.cache_path(), cache_path);
}

TEST_F(FootageProbeTest, SecondProbeReadsBackMetadataCache)
{
	const QString path = DemoVideoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::Footage *first = AddProbedFootage(path);
	ASSERT_TRUE(first->IsValid());

	// The first probe writes a stream metadata cache into the cache location
	const QString cache_file = MetadataCacheFileFor(path);
	ASSERT_TRUE(QFileInfo::exists(cache_file));

	// A second footage for the same file loads its metadata from that cache
	// and ends up with identical state
	olive::Footage *second = AddProbedFootage(path);
	ASSERT_TRUE(second->IsValid());
	EXPECT_EQ(second->decoder(), first->decoder());
	EXPECT_EQ(second->GetTotalStreamCount(), first->GetTotalStreamCount());
	EXPECT_EQ(second->GetVideoStreamCount(), first->GetVideoStreamCount());
	EXPECT_EQ(second->GetAudioStreamCount(), first->GetAudioStreamCount());

	const olive::VideoParams from_cache = second->GetVideoParams(0);
	const olive::VideoParams probed = first->GetVideoParams(0);
	EXPECT_EQ(from_cache.stream_index(), probed.stream_index());
	EXPECT_EQ(from_cache.width(), probed.width());
	EXPECT_EQ(from_cache.height(), probed.height());
	EXPECT_EQ(from_cache.frame_rate(), probed.frame_rate());
	EXPECT_EQ(from_cache.time_base(), probed.time_base());
	EXPECT_EQ(from_cache.duration(), probed.duration());
	EXPECT_EQ(from_cache.video_type(), probed.video_type());

	ASSERT_TRUE(second->HasSourceStartTime());
	EXPECT_EQ(second->source_start_time(), olive::rational(3600));
	EXPECT_EQ(second->source_start_time_source(), QStringLiteral("timecode"));
}

TEST_F(FootageProbeTest, FilenameChangeToMissingFileClearsProbeState)
{
	const QString path = DemoVideoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::Footage *footage = AddProbedFootage(path);
	ASSERT_TRUE(footage->IsValid());
	ASSERT_GT(footage->GetTotalStreamCount(), 0);

	// Pointing the footage at a nonexistent file clears the probed state and
	// the re-probe fails
	footage->set_filename(
		QDir(temp_dir_.path()).filePath(QStringLiteral("gone.mp4")));

	EXPECT_FALSE(footage->IsValid());
	EXPECT_EQ(footage->GetTotalStreamCount(), 0);
	EXPECT_EQ(footage->GetVideoStreamCount(), 0);
	EXPECT_EQ(footage->GetAudioStreamCount(), 0);
	EXPECT_TRUE(footage->decoder().isEmpty());
	EXPECT_EQ(footage->timestamp(), 0);
	EXPECT_FALSE(footage->HasSourceStartTime());
}

TEST_F(FootageProbeTest, CheckFootageOnlyRespondsWithActiveWindow)
{
	const QString path = TestImagePath();
	ASSERT_TRUE(QFileInfo::exists(path));

	// Work on a copy so the original test asset is untouched
	const QString copy =
		QDir(temp_dir_.path()).filePath(QStringLiteral("image.png"));
	ASSERT_TRUE(QFile::copy(path, copy));

	olive::Footage *footage = AddProbedFootage(copy);
	ASSERT_TRUE(footage->IsValid());
	const qint64 probed_timestamp = footage->timestamp();
	ASSERT_GT(probed_timestamp, 0);

	// The file vanishes behind the footage's back
	ASSERT_TRUE(QFile::remove(copy));

	// Without an active window, CheckFootage is a no-op
	ASSERT_TRUE(
		QMetaObject::invokeMethod(footage, "CheckFootage", Qt::DirectConnection));
	EXPECT_EQ(footage->timestamp(), probed_timestamp);
	EXPECT_TRUE(footage->IsValid());

	// With an active window, CheckFootage notices the missing file and
	// re-probes. The re-probe clears the existing state first, and since the
	// file no longer exists, the footage is left invalid with no streams.
	{
		QWidget window;
		window.show();
		window.activateWindow();
		QCoreApplication::processEvents();
		ASSERT_EQ(qApp->activeWindow(), &window);

		ASSERT_TRUE(QMetaObject::invokeMethod(footage, "CheckFootage",
											  Qt::DirectConnection));
	}
	ASSERT_EQ(qApp->activeWindow(), nullptr);

	EXPECT_EQ(footage->timestamp(), 0);
	EXPECT_FALSE(footage->IsValid());
	EXPECT_EQ(footage->GetVideoStreamCount(), 0);
}

TEST_F(FootageProbeTest, ProbingExistingButInvalidMediaStaysInvalid)
{
	const QString path =
		QDir(temp_dir_.path()).filePath(QStringLiteral("fake.mkv"));
	{
		QFile file(path);
		ASSERT_TRUE(file.open(QFile::WriteOnly));
		file.write("OAK_FAKE_MEDIA");
	}

	olive::Footage *footage = AddProbedFootage(path);

	// The file exists but no decoder can probe it, so the footage stays
	// invalid
	EXPECT_FALSE(footage->IsValid());
	EXPECT_TRUE(footage->decoder().isEmpty());
	EXPECT_EQ(footage->GetTotalStreamCount(), 0);
	EXPECT_EQ(footage->GetVideoStreamCount(), 0);
	EXPECT_EQ(footage->GetAudioStreamCount(), 0);
	EXPECT_EQ(footage->GetSubtitleStreamCount(), 0);

	// A failed probe is not written to the metadata cache, so future reprobes
	// of the same path probe again instead of reloading an invalid description
	EXPECT_FALSE(QFileInfo::exists(MetadataCacheFileFor(path)));
}
