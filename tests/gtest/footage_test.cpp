#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/filefunctions.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/globals.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "render/diskmanager.h"
#include "node/project/footage/footagedescription.h"
#include "render/job/footagejob.h"
#include "render/loopmode.h"
#include "render/texture.h"

namespace
{

// Footage subclass that exposes the protected stream mutators of ViewerOutput
// so tests can populate streams without probing real media
class TestableFootage : public olive::Footage {
public:
	using olive::ViewerOutput::AddStream;
	using olive::ViewerOutput::SetStream;
};

// Temporarily overrides an environment variable, restoring the previous state
// on destruction. Used to sandbox the footage metadata cache inside a
// QTemporaryDir.
class ScopedEnvVar {
public:
	ScopedEnvVar(const char *name, const QByteArray &value)
		: name_(name)
		, old_value_(qgetenv(name))
		, had_value_(qEnvironmentVariableIsSet(name))
	{
		qputenv(name_, value);
	}

	~ScopedEnvVar()
	{
		if (had_value_) {
			qputenv(name_, old_value_);
		} else {
			qunsetenv(name_);
		}
	}

private:
	const char *name_;
	QByteArray old_value_;
	bool had_value_;
};

olive::VideoParams MakeVideoStream(int stream_index)
{
	olive::VideoParams params(1920, 1080, olive::rational(1, 24),
							  olive::core::PixelFormat::U8, 4);
	params.set_stream_index(stream_index);
	params.set_duration(48); // 2 seconds at 24 fps
	return params;
}

olive::core::AudioParams MakeAudioStream(int stream_index)
{
	olive::core::AudioParams params(48000, olive::core::kChannelLayoutStereo,
									olive::core::SampleFormat::F32P);
	params.set_stream_index(stream_index);
	params.set_duration(96000); // 2 seconds at 48 kHz
	return params;
}

olive::SubtitleParams MakeSubtitleStream(int stream_index)
{
	olive::SubtitleParams params;
	params.set_stream_index(stream_index);
	params.push_back(olive::Subtitle(
		olive::TimeRange(olive::rational(0), olive::rational(3)),
		QStringLiteral("subtitle text")));
	return params;
}

// Two video streams (one with an explicit colorspace, one without) and one
// audio stream, reported as three source streams in total
olive::FootageDescription MakeStandardDescription()
{
	olive::FootageDescription desc(QStringLiteral("fakedecoder"));

	olive::VideoParams video0 = MakeVideoStream(0);
	desc.AddVideoStream(video0);

	olive::VideoParams video1(1280, 720, olive::rational(1, 24),
							  olive::core::PixelFormat::U8, 4);
	video1.set_stream_index(1);
	video1.set_duration(48);
	video1.set_colorspace(QStringLiteral("ExplicitSpace"));
	desc.AddVideoStream(video1);

	desc.AddAudioStream(MakeAudioStream(2));

	desc.SetStreamCount(3);
	return desc;
}

QString CreateFakeMediaFile(QTemporaryDir &dir, const QString &name)
{
	const QString path = QDir(dir.path()).filePath(name);
	QFile file(path);
	if (!file.open(QFile::WriteOnly)) {
		return QString();
	}
	file.write("OAK_FAKE_MEDIA");
	file.close();
	return path;
}

// Seeds the footage metadata cache for media_path with desc, then points a
// project-parented Footage at the file so Reprobe() picks the cache up without
// running any real decoders. The caller is responsible for redirecting
// QStandardPaths::CacheLocation into a temporary directory first. Returns
// nullptr if the cache file could not be written.
TestableFootage *ProbeFootageFromCache(olive::Project *project,
									   const QString &media_path,
									   const olive::FootageDescription &desc)
{
	const QString cache_location =
		QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	if (cache_location.isEmpty() || !QDir().mkpath(cache_location)) {
		return nullptr;
	}

	const QString cache_file =
		QDir(cache_location)
			.filePath(olive::FileFunctions::GetUniqueFileIdentifier(media_path));
	if (!desc.Save(cache_file)) {
		return nullptr;
	}

	auto *footage = new TestableFootage();
	footage->setParent(project);
	footage->set_filename(media_path);
	return footage;
}

olive::NodeGlobals MakeGlobals(
	olive::LoopMode loop_mode = olive::LoopMode::kLoopModeOff, int divider = 1)
{
	olive::VideoParams vparams(64, 64, olive::rational(1, 24),
							   olive::core::PixelFormat::U8, 4);
	vparams.set_divider(divider);
	return olive::NodeGlobals(vparams, olive::core::AudioParams(),
							  olive::rational(0), loop_mode);
}

} // namespace

TEST(FootageStatic, DescribeVideoStreamFormatsVideoAndStillStreams)
{
	olive::VideoParams video = MakeVideoStream(0);
	EXPECT_EQ(olive::Footage::DescribeVideoStream(video),
			  QStringLiteral("0: Video - 1920x1080"));

	video.set_video_type(olive::VideoParams::kVideoTypeStill);
	video.set_stream_index(3);
	EXPECT_EQ(olive::Footage::DescribeVideoStream(video),
			  QStringLiteral("3: Image - 1920x1080"));
}

TEST(FootageStatic, DescribeAudioStreamContainsIndexAndRate)
{
	olive::core::AudioParams audio = MakeAudioStream(1);

	// The %n plural marker is only substituted when a translation is loaded,
	// so assert on the stable parts of the description instead
	const QString description = olive::Footage::DescribeAudioStream(audio);
	EXPECT_TRUE(description.startsWith(QStringLiteral("1: Audio")));
	EXPECT_TRUE(description.contains(QStringLiteral("48000Hz")));
}

TEST(FootageStatic, DescribeSubtitleStreamContainsIndex)
{
	olive::SubtitleParams subs = MakeSubtitleStream(4);
	EXPECT_EQ(olive::Footage::DescribeSubtitleStream(subs),
			  QStringLiteral("4: Subtitle"));
}

TEST(FootageStatic, GetStreamTypeNameCoversAllTrackTypes)
{
	EXPECT_EQ(olive::Footage::GetStreamTypeName(olive::Track::kVideo),
			  QStringLiteral("Video"));
	EXPECT_EQ(olive::Footage::GetStreamTypeName(olive::Track::kAudio),
			  QStringLiteral("Audio"));
	EXPECT_EQ(olive::Footage::GetStreamTypeName(olive::Track::kSubtitle),
			  QStringLiteral("Subtitle"));
	EXPECT_EQ(olive::Footage::GetStreamTypeName(olive::Track::kNone),
			  QStringLiteral("Unknown"));
	EXPECT_EQ(olive::Footage::GetStreamTypeName(olive::Track::kCount),
			  QStringLiteral("Unknown"));
}

TEST(FootageStatic, AdjustTimeByLoopModeReturnsZeroForStillImages)
{
	// Still images never loop, clamp, or drop: the adjusted time is always 0,
	// even for in-bounds times
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(3), olive::LoopMode::kLoopModeOff,
				  olive::rational(10), olive::VideoParams::kVideoTypeStill,
				  olive::rational(1, 24)),
			  olive::rational(0));
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(30), olive::LoopMode::kLoopModeLoop,
				  olive::rational(10), olive::VideoParams::kVideoTypeStill,
				  olive::rational(1, 24)),
			  olive::rational(0));
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(-1), olive::LoopMode::kLoopModeClamp,
				  olive::rational(10), olive::VideoParams::kVideoTypeStill,
				  olive::rational(1, 24)),
			  olive::rational(0));
}

TEST(FootageStatic, AdjustTimeByLoopModeKeepsInBoundsTime)
{
	for (olive::LoopMode mode : { olive::LoopMode::kLoopModeOff,
								  olive::LoopMode::kLoopModeClamp,
								  olive::LoopMode::kLoopModeLoop }) {
		EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
					  olive::rational(3), mode, olive::rational(10),
					  olive::VideoParams::kVideoTypeVideo,
					  olive::rational(1, 24)),
				  olive::rational(3));
	}
}

TEST(FootageStatic, AdjustTimeByLoopModeOffDropsOutOfBoundsTime)
{
	const olive::rational negative = olive::Footage::AdjustTimeByLoopMode(
		olive::rational(-1), olive::LoopMode::kLoopModeOff, olive::rational(10),
		olive::VideoParams::kVideoTypeVideo, olive::rational(1, 24));
	EXPECT_TRUE(negative.isNaN());

	// The length itself is already out of bounds
	const olive::rational at_length = olive::Footage::AdjustTimeByLoopMode(
		olive::rational(10), olive::LoopMode::kLoopModeOff, olive::rational(10),
		olive::VideoParams::kVideoTypeVideo, olive::rational(1, 24));
	EXPECT_TRUE(at_length.isNaN());
}

TEST(FootageStatic, AdjustTimeByLoopModeClampsToLength)
{
	// Beyond the end, clamp to the last frame (length - timebase)
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(10), olive::LoopMode::kLoopModeClamp,
				  olive::rational(10), olive::VideoParams::kVideoTypeVideo,
				  olive::rational(1, 24)),
			  olive::rational(239, 24));

	// Before the start, clamp to 0
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(-5), olive::LoopMode::kLoopModeClamp,
				  olive::rational(10), olive::VideoParams::kVideoTypeVideo,
				  olive::rational(1, 24)),
			  olive::rational(0));
}

TEST(FootageStatic, AdjustTimeByLoopModeLoopsAroundLength)
{
	// Single wrap past the end
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(12), olive::LoopMode::kLoopModeLoop,
				  olive::rational(10), olive::VideoParams::kVideoTypeVideo,
				  olive::rational(1, 24)),
			  olive::rational(2));

	// Multiple wraps past the end
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(25), olive::LoopMode::kLoopModeLoop,
				  olive::rational(10), olive::VideoParams::kVideoTypeVideo,
				  olive::rational(1, 24)),
			  olive::rational(5));

	// Wraps from before the start
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(-3), olive::LoopMode::kLoopModeLoop,
				  olive::rational(10), olive::VideoParams::kVideoTypeVideo,
				  olive::rational(1, 24)),
			  olive::rational(7));
	EXPECT_EQ(olive::Footage::AdjustTimeByLoopMode(
				  olive::rational(-25), olive::LoopMode::kLoopModeLoop,
				  olive::rational(10), olive::VideoParams::kVideoTypeVideo,
				  olive::rational(1, 24)),
			  olive::rational(5));
}

TEST(FootageStatic, AdjustTimeByLoopModeWithEmptyRangeReturnsNaN)
{
	// Looping an empty range would never terminate; return NaN instead
	EXPECT_TRUE(olive::Footage::AdjustTimeByLoopMode(
					olive::rational(1), olive::LoopMode::kLoopModeLoop,
					olive::rational(0), olive::VideoParams::kVideoTypeVideo,
					olive::rational(1, 24))
					.isNaN());

	// Clamping a range shorter than one frame has no frame to clamp to
	EXPECT_TRUE(olive::Footage::AdjustTimeByLoopMode(
					olive::rational(1), olive::LoopMode::kLoopModeClamp,
					olive::rational(0), olive::VideoParams::kVideoTypeVideo,
					olive::rational(1, 24))
					.isNaN());
}

TEST(FootageStatic, RetranslateSetsInputNames)
{
	TestableFootage footage;
	footage.Retranslate();

	EXPECT_EQ(footage.GetInputName(olive::Footage::kFilenameInput),
			  QStringLiteral("Filename"));
	EXPECT_EQ(footage.GetInputName(olive::ViewerOutput::kVideoParamsInput),
			  QStringLiteral("Video Parameters"));
	EXPECT_EQ(footage.GetInputName(olive::ViewerOutput::kAudioParamsInput),
			  QStringLiteral("Audio Parameters"));
	EXPECT_EQ(footage.GetInputName(olive::ViewerOutput::kSubtitleParamsInput),
			  QStringLiteral("Subtitle Parameters"));
}

class FootageTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!olive::Core::instance()) {
			// Leaked intentionally: Core is process-wide and DiskManager
			// touches it (matches render_diskcache_test).
			new olive::Core(olive::Core::CoreParams());
		}

		// Footage::Value() resolves Project::cache_path(), which goes through
		// the DiskManager singleton
		olive::DiskManager::CreateInstance();

		olive::ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	void TearDown() override
	{
		project_.reset();
		olive::DiskManager::DestroyInstance();
	}

	olive::Footage *AddFootage()
	{
		auto *footage = new olive::Footage();
		footage->setParent(project_.get());
		return footage;
	}

	std::unique_ptr<olive::Project> project_;
};

TEST_F(FootageTest, ManuallyAddedStreamsMapBetweenReferencesAndIndices)
{
	TestableFootage footage;

	EXPECT_EQ(footage.AddStream(olive::Track::kVideo,
								QVariant::fromValue(MakeVideoStream(5))),
			  0);
	EXPECT_EQ(footage.AddStream(olive::Track::kAudio,
								QVariant::fromValue(MakeAudioStream(2))),
			  0);
	EXPECT_EQ(footage.AddStream(olive::Track::kSubtitle,
								QVariant::fromValue(MakeSubtitleStream(7))),
			  0);

	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kVideo, 0), 5);
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kAudio, 0), 2);
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kSubtitle, 0), 7);
	EXPECT_EQ(footage.GetStreamIndex(
				  olive::Track::Reference(olive::Track::kVideo, 0)),
			  5);
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kNone, 0), -1);
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kCount, 0), -1);

	// Out-of-range indices report -1 rather than a default-constructed stream
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kVideo, 1), -1);
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kVideo, -1), -1);
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kAudio, 1), -1);
	EXPECT_EQ(footage.GetStreamIndex(olive::Track::kSubtitle, 1), -1);

	EXPECT_EQ(footage.GetReferenceFromRealIndex(5),
			  olive::Track::Reference(olive::Track::kVideo, 0));
	EXPECT_EQ(footage.GetReferenceFromRealIndex(2),
			  olive::Track::Reference(olive::Track::kAudio, 0));
	EXPECT_EQ(footage.GetReferenceFromRealIndex(7),
			  olive::Track::Reference(olive::Track::kSubtitle, 0));

	const olive::Track::Reference unknown =
		footage.GetReferenceFromRealIndex(99);
	EXPECT_EQ(unknown.type(), olive::Track::kNone);
	EXPECT_EQ(unknown.index(), -1);
}

TEST_F(FootageTest, ConnectedOutputsReflectStreamTypes)
{
	TestableFootage footage;

	EXPECT_EQ(footage.GetConnectedTextureOutput(), nullptr);
	EXPECT_EQ(footage.GetConnectedSampleOutput(), nullptr);

	footage.AddStream(olive::Track::kVideo,
					  QVariant::fromValue(MakeVideoStream(0)));
	EXPECT_EQ(footage.GetConnectedTextureOutput(),
			  static_cast<olive::Node *>(&footage));
	EXPECT_EQ(footage.GetConnectedSampleOutput(), nullptr);

	footage.AddStream(olive::Track::kAudio,
					  QVariant::fromValue(MakeAudioStream(1)));
	EXPECT_EQ(footage.GetConnectedSampleOutput(),
			  static_cast<olive::Node *>(&footage));
}

TEST_F(FootageTest, DataRolesForInvalidFootage)
{
	TestableFootage footage;

	EXPECT_EQ(footage.data(olive::Node::TOOLTIP).toString(),
			  QStringLiteral("Invalid"));
	EXPECT_TRUE(footage.data(olive::Node::ICON).canConvert<QIcon>());

	// With no existing file behind the footage, the time roles fall through
	// to the base class and stay invalid
	EXPECT_FALSE(footage.data(olive::Node::CREATED_TIME).isValid());
	EXPECT_FALSE(footage.data(olive::Node::MODIFIED_TIME).isValid());
}

TEST_F(FootageTest, TooltipDescribesEnabledStreams)
{
	TestableFootage footage;
	// The file does not exist, so the filename change clears the footage
	// without probing anything
	footage.set_filename(QStringLiteral("/nonexistent/media.mkv"));
	footage.AddStream(olive::Track::kVideo,
					  QVariant::fromValue(MakeVideoStream(0)));
	footage.AddStream(olive::Track::kAudio,
					  QVariant::fromValue(MakeAudioStream(1)));
	footage.SetValid();

	QString tip = footage.data(olive::Node::TOOLTIP).toString();
	EXPECT_TRUE(
		tip.contains(QStringLiteral("Filename: /nonexistent/media.mkv")));
	EXPECT_TRUE(tip.contains(QStringLiteral("0: Video - 1920x1080")));
	EXPECT_TRUE(tip.contains(QStringLiteral("Audio")));

	// Disabled streams are omitted from the tooltip
	olive::VideoParams disabled = footage.GetVideoParams(0);
	disabled.set_enabled(false);
	footage.SetStream(olive::Track::kVideo, QVariant::fromValue(disabled), 0);

	tip = footage.data(olive::Node::TOOLTIP).toString();
	EXPECT_FALSE(tip.contains(QStringLiteral("0: Video")));
	EXPECT_TRUE(tip.contains(QStringLiteral("Audio")));
}

TEST_F(FootageTest, IconReflectsPrioritizedStreamType)
{
	// Invalid footage
	TestableFootage footage;
	EXPECT_TRUE(footage.data(olive::Node::ICON).canConvert<QIcon>());

	// Real video streams take priority over audio
	footage.AddStream(olive::Track::kVideo,
					  QVariant::fromValue(MakeVideoStream(0)));
	footage.AddStream(olive::Track::kAudio,
					  QVariant::fromValue(MakeAudioStream(1)));
	footage.SetValid();
	EXPECT_TRUE(footage.data(olive::Node::ICON).canConvert<QIcon>());

	// A still image without audio hits the image branch
	TestableFootage stills;
	olive::VideoParams still = MakeVideoStream(0);
	still.set_video_type(olive::VideoParams::kVideoTypeStill);
	stills.AddStream(olive::Track::kVideo, QVariant::fromValue(still));
	stills.SetValid();
	EXPECT_TRUE(stills.data(olive::Node::ICON).canConvert<QIcon>());

	// Audio-only footage
	TestableFootage audio_only;
	audio_only.AddStream(olive::Track::kAudio,
						 QVariant::fromValue(MakeAudioStream(0)));
	audio_only.SetValid();
	EXPECT_TRUE(audio_only.data(olive::Node::ICON).canConvert<QIcon>());

	// Subtitle-only footage
	TestableFootage subs_only;
	subs_only.AddStream(olive::Track::kSubtitle,
						QVariant::fromValue(MakeSubtitleStream(0)));
	subs_only.SetValid();
	EXPECT_TRUE(subs_only.data(olive::Node::ICON).canConvert<QIcon>());
}

TEST_F(FootageTest, ProxyChangesMarkProjectModifiedAndEmitSignal)
{
	olive::Footage *footage = AddFootage();
	ASSERT_FALSE(project_->is_modified());

	int emissions = 0;
	QObject::connect(footage, &olive::Footage::ProxySettingsChanged,
					 [&emissions]() { ++emissions; });

	footage->SetProxy(QStringLiteral("/cache/proxy/example.mp4"),
					  olive::ProxyManager::kProxyReady, 0, 1, true);
	EXPECT_EQ(emissions, 1);
	EXPECT_TRUE(project_->is_modified());

	// set_proxy_enabled only reacts to actual changes
	project_->set_modified(false);
	footage->set_proxy_enabled(true);
	EXPECT_EQ(emissions, 1);
	EXPECT_FALSE(project_->is_modified());

	footage->set_proxy_enabled(false);
	EXPECT_EQ(emissions, 2);
	EXPECT_TRUE(project_->is_modified());
	EXPECT_FALSE(footage->proxy_enabled());
}

TEST_F(FootageTest, ReprobeRestoresStreamsFromMetadataCache)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const ScopedEnvVar xdg(
		"XDG_CACHE_HOME",
		QDir(dir.path()).filePath(QStringLiteral("xdg")).toUtf8());

	const QString media = CreateFakeMediaFile(dir, QStringLiteral("fake.mkv"));
	ASSERT_FALSE(media.isEmpty());

	TestableFootage *footage = ProbeFootageFromCache(
		project_.get(), media, MakeStandardDescription());
	ASSERT_NE(footage, nullptr);

	EXPECT_TRUE(footage->IsValid());
	EXPECT_EQ(footage->decoder(), QStringLiteral("fakedecoder"));
	EXPECT_EQ(footage->GetTotalStreamCount(), 3);
	EXPECT_EQ(footage->GetVideoStreamCount(), 2);
	EXPECT_EQ(footage->GetAudioStreamCount(), 1);
	EXPECT_EQ(footage->GetSubtitleStreamCount(), 0);

	EXPECT_EQ(footage->GetStreamIndex(olive::Track::kVideo, 0), 0);
	EXPECT_EQ(footage->GetStreamIndex(olive::Track::kVideo, 1), 1);
	EXPECT_EQ(footage->GetStreamIndex(olive::Track::kAudio, 0), 2);
	EXPECT_EQ(footage->GetReferenceFromRealIndex(2),
			  olive::Track::Reference(olive::Track::kAudio, 0));

	EXPECT_EQ(footage->GetConnectedTextureOutput(),
			  static_cast<olive::Node *>(footage));
	EXPECT_EQ(footage->GetConnectedSampleOutput(),
			  static_cast<olive::Node *>(footage));

	// The file behind the footage exists, so both time roles are reported
	const QVariant modified = footage->data(olive::Node::MODIFIED_TIME);
	ASSERT_TRUE(modified.isValid());
	EXPECT_EQ(modified.toLongLong(),
			  QFileInfo(media).lastModified().toSecsSinceEpoch());
	EXPECT_TRUE(footage->data(olive::Node::CREATED_TIME).isValid());
}

TEST_F(FootageTest, VerifyLengthUsesStreamDurations)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const ScopedEnvVar xdg(
		"XDG_CACHE_HOME",
		QDir(dir.path()).filePath(QStringLiteral("xdg")).toUtf8());

	const QString media = CreateFakeMediaFile(dir, QStringLiteral("fake.mkv"));
	ASSERT_FALSE(media.isEmpty());

	TestableFootage *footage = ProbeFootageFromCache(
		project_.get(), media, MakeStandardDescription());
	ASSERT_NE(footage, nullptr);

	// Both streams describe two seconds of media
	footage->VerifyLength();
	EXPECT_EQ(footage->GetVideoLength(), olive::rational(2));
	EXPECT_EQ(footage->GetAudioLength(), olive::rational(2));
	EXPECT_EQ(footage->GetLength(), olive::rational(2));
}

TEST_F(FootageTest, ValueSkipsMissingFiles)
{
	TestableFootage footage;

	olive::NodeValueRow row;
	row.insert(olive::Footage::kFilenameInput,
			   olive::NodeValue(olive::NodeValue::kFile,
								QStringLiteral("/nonexistent/media.mkv")));

	olive::NodeValueTable table;
	footage.Value(row, MakeGlobals(), &table);

	EXPECT_TRUE(table.isEmpty());
}

TEST_F(FootageTest, ValuePushesOnlyLengthWhenNoStreams)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString media = CreateFakeMediaFile(dir, QStringLiteral("empty.mkv"));
	ASSERT_FALSE(media.isEmpty());

	TestableFootage footage;

	olive::NodeValueRow row;
	row.insert(olive::Footage::kFilenameInput,
			   olive::NodeValue(olive::NodeValue::kFile, media));

	olive::NodeValueTable table;
	footage.Value(row, MakeGlobals(), &table);

	// The file exists but no streams were ever probed, so only the (zero)
	// length is pushed
	ASSERT_EQ(table.Count(), 1);
	const olive::NodeValue length =
		table.Get(olive::NodeValue::kRational, QStringLiteral("length"));
	EXPECT_EQ(length.type(), olive::NodeValue::kRational);
	EXPECT_EQ(length.toRational(), olive::rational(0));
	EXPECT_EQ(length.source(), static_cast<const olive::Node *>(&footage));
}

TEST_F(FootageTest, ValuePushesStreamJobs)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const ScopedEnvVar xdg(
		"XDG_CACHE_HOME",
		QDir(dir.path()).filePath(QStringLiteral("xdg")).toUtf8());

	const QString media = CreateFakeMediaFile(dir, QStringLiteral("fake.mkv"));
	ASSERT_FALSE(media.isEmpty());

	TestableFootage *footage = ProbeFootageFromCache(
		project_.get(), media, MakeStandardDescription());
	ASSERT_NE(footage, nullptr);
	footage->VerifyLength();

	// The colorspace fallback reads the project default, and the audio cache
	// path comes from the project's cache settings
	project_->SetDefaultInputColorSpace(QStringLiteral("TestInputSpace"));
	project_->SetCacheLocationSetting(olive::Project::kCacheCustomPath);
	const QString cache_path =
		QDir(dir.path()).filePath(QStringLiteral("cache"));
	project_->SetCustomCachePath(cache_path);

	olive::NodeValueRow row;
	row.insert(olive::Footage::kFilenameInput,
			   olive::NodeValue(olive::NodeValue::kFile, media));

	// A divider > 1 routes through the target-resolution divider calculation
	const olive::NodeGlobals globals =
		MakeGlobals(olive::LoopMode::kLoopModeLoop, 2);

	olive::NodeValueTable table;
	footage->Value(row, globals, &table);

	// Length, two texture jobs, and one sample job
	EXPECT_EQ(table.Count(), 4);

	const olive::NodeValue length =
		table.Get(olive::NodeValue::kRational, QStringLiteral("length"));
	EXPECT_EQ(length.toRational(), olive::rational(2));

	// A stream without a colorspace falls back to the project default
	const olive::TexturePtr tex0 =
		table.Get(olive::NodeValue::kTexture, QStringLiteral("v:0"))
			.toTexture();
	ASSERT_NE(tex0, nullptr);
	EXPECT_EQ(tex0->params().colorspace(), QStringLiteral("TestInputSpace"));
	// min(calculated divider for 32x32 from 1920x1080, requested 2)
	EXPECT_EQ(tex0->params().divider(), 2);

	// An explicit colorspace survives
	const olive::TexturePtr tex1 =
		table.Get(olive::NodeValue::kTexture, QStringLiteral("v:1"))
			.toTexture();
	ASSERT_NE(tex1, nullptr);
	EXPECT_EQ(tex1->params().colorspace(), QStringLiteral("ExplicitSpace"));

	const olive::NodeValue samples =
		table.Get(olive::NodeValue::kSamples, QStringLiteral("a:0"));
	ASSERT_EQ(samples.type(), olive::NodeValue::kSamples);
	const olive::FootageJob audio_job =
		samples.data().value<olive::FootageJob>();
	EXPECT_EQ(audio_job.filename(), media);
	EXPECT_EQ(audio_job.decoder(), QStringLiteral("fakedecoder"));
	EXPECT_EQ(audio_job.type(), olive::Track::kAudio);
	EXPECT_EQ(audio_job.audio_params().sample_rate(), 48000);
	EXPECT_EQ(audio_job.length(), olive::rational(2));
	EXPECT_EQ(audio_job.loop_mode(), olive::LoopMode::kLoopModeLoop);
	EXPECT_EQ(audio_job.time(), globals.time());
	EXPECT_EQ(audio_job.cache_path(), cache_path);

	// With a divider of 1, everything renders at full resolution
	olive::NodeValueTable full_res;
	footage->Value(row, MakeGlobals(), &full_res);
	const olive::TexturePtr full_res_tex =
		full_res.Get(olive::NodeValue::kTexture, QStringLiteral("v:0"))
			.toTexture();
	ASSERT_NE(full_res_tex, nullptr);
	EXPECT_EQ(full_res_tex->params().divider(), 1);
}

TEST_F(FootageTest, ValueAttachesReadyProxyToJobs)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const ScopedEnvVar xdg(
		"XDG_CACHE_HOME",
		QDir(dir.path()).filePath(QStringLiteral("xdg")).toUtf8());

	const QString media = CreateFakeMediaFile(dir, QStringLiteral("fake.mkv"));
	ASSERT_FALSE(media.isEmpty());

	TestableFootage *footage = ProbeFootageFromCache(
		project_.get(), media, MakeStandardDescription());
	ASSERT_NE(footage, nullptr);
	footage->VerifyLength();

	// A ready proxy is simply an existing proxy file with no .working
	// sibling; the .a1. marker declares that it contains audio
	const QString proxy = QDir(dir.path())
							  .filePath(QStringLiteral(
								  "proxy-0.1920x1080.v1.a1.mp4"));
	{
		QFile proxy_file(proxy);
		ASSERT_TRUE(proxy_file.open(QFile::WriteOnly));
	}
	footage->SetProxy(proxy, olive::ProxyManager::kProxyReady, 0, 1, true);

	olive::NodeValueRow row;
	row.insert(olive::Footage::kFilenameInput,
			   olive::NodeValue(olive::NodeValue::kFile, media));

	olive::NodeValueTable table;
	footage->Value(row, MakeGlobals(), &table);

	// The proxied video stream (real index 0) gets the proxy at stream 0
	const olive::TexturePtr tex0 =
		table.Get(olive::NodeValue::kTexture, QStringLiteral("v:0"))
			.toTexture();
	ASSERT_NE(tex0, nullptr);
	const auto *video0_job =
		static_cast<const olive::FootageJob *>(tex0->job());
	ASSERT_NE(video0_job, nullptr);
	EXPECT_TRUE(video0_job->has_proxy());
	EXPECT_EQ(video0_job->proxy_filename(), proxy);
	EXPECT_EQ(video0_job->proxy_decoder(), QStringLiteral("ffmpeg"));
	EXPECT_EQ(video0_job->proxy_stream_index(), 0);

	// Other video streams are unaffected
	const olive::TexturePtr tex1 =
		table.Get(olive::NodeValue::kTexture, QStringLiteral("v:1"))
			.toTexture();
	ASSERT_NE(tex1, nullptr);
	const auto *video1_job =
		static_cast<const olive::FootageJob *>(tex1->job());
	ASSERT_NE(video1_job, nullptr);
	EXPECT_FALSE(video1_job->has_proxy());

	// The first audio stream follows the video stream inside the proxy file
	const olive::FootageJob audio_job =
		table.Get(olive::NodeValue::kSamples, QStringLiteral("a:0"))
			.data()
			.value<olive::FootageJob>();
	EXPECT_TRUE(audio_job.has_proxy());
	EXPECT_EQ(audio_job.proxy_filename(), proxy);
	EXPECT_EQ(audio_job.proxy_stream_index(), 1);

	// Disabling the proxy detaches it from subsequent jobs
	footage->set_proxy_enabled(false);
	olive::NodeValueTable no_proxy;
	footage->Value(row, MakeGlobals(), &no_proxy);
	const olive::TexturePtr no_proxy_tex =
		no_proxy.Get(olive::NodeValue::kTexture, QStringLiteral("v:0"))
			.toTexture();
	ASSERT_NE(no_proxy_tex, nullptr);
	const auto *no_proxy_job =
		static_cast<const olive::FootageJob *>(no_proxy_tex->job());
	ASSERT_NE(no_proxy_job, nullptr);
	EXPECT_FALSE(no_proxy_job->has_proxy());
}

TEST_F(FootageTest, SaveCustomPersistsSourceStartTime)
{
	olive::Footage footage;
	footage.set_timestamp(7);
	footage.SetSourceStartTime(olive::rational(3600), QStringLiteral("manual"));

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	footage.SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	EXPECT_TRUE(xml.contains(QStringLiteral("sourcestarttime")));
	EXPECT_TRUE(xml.contains(QStringLiteral("source=\"manual\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("3600/1")));

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));

	olive::Footage loaded;
	ASSERT_TRUE(loaded.LoadCustom(&reader, nullptr));
	ASSERT_TRUE(loaded.HasSourceStartTime());
	EXPECT_EQ(loaded.source_start_time(), olive::rational(3600));
	EXPECT_EQ(loaded.source_start_time_source(), QStringLiteral("manual"));
	EXPECT_EQ(loaded.timestamp(), 7);
}
