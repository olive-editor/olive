#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "codec/proxymanager.h"
#include "config/config.h"
#include "node/project/footage/footage.h"
#include "render/job/footagejob.h"
#include "task/proxy/proxy.h"

namespace
{

QVariant ProxyConfigValue(const char *key)
{
	return olive::Config::Current()[QString::fromUtf8(key)];
}

} // namespace

TEST(ProxyManager, BuildsStableProxyFilename)
{
	olive::ProxyManager::ProxyParams params;
	params.width = 1280;
	params.height = 720;
	params.version = 1;

	const QString first = olive::ProxyManager::GetProxyFilename(
		QStringLiteral("/tmp/oak-cache"), QStringLiteral("/media/source.mov"),
		0, params);
	const QString second = olive::ProxyManager::GetProxyFilename(
		QStringLiteral("/tmp/oak-cache"), QStringLiteral("/media/source.mov"),
		0, params);
	const QString other_stream = olive::ProxyManager::GetProxyFilename(
		QStringLiteral("/tmp/oak-cache"), QStringLiteral("/media/source.mov"),
		1, params);

	EXPECT_EQ(first, second);
	EXPECT_NE(first, other_stream);
	EXPECT_TRUE(first.contains(QStringLiteral("/proxy/")));
	EXPECT_TRUE(first.endsWith(QStringLiteral(".mp4")));
}

TEST(ProxyManager, ProxyFilenameIncludesPresetParameters)
{
	olive::ProxyManager::ProxyParams mp4_720p;
	mp4_720p.width = 1280;
	mp4_720p.height = 720;
	mp4_720p.version = 1;
	mp4_720p.extension = QStringLiteral("mp4");

	olive::ProxyManager::ProxyParams mov_540p = mp4_720p;
	mov_540p.width = 960;
	mov_540p.height = 540;
	mov_540p.version = 2;
	mov_540p.extension = QStringLiteral("mov");

	const QString first = olive::ProxyManager::GetProxyFilename(
		QStringLiteral("/tmp/oak-cache"), QStringLiteral("/media/source.mov"),
		0, mp4_720p);
	const QString second = olive::ProxyManager::GetProxyFilename(
		QStringLiteral("/tmp/oak-cache"), QStringLiteral("/media/source.mov"),
		0, mov_540p);

	EXPECT_NE(first, second);
	EXPECT_TRUE(first.contains(QStringLiteral(".1280x720.v1.")));
	EXPECT_TRUE(second.contains(QStringLiteral(".960x540.v2.")));
	EXPECT_TRUE(second.endsWith(QStringLiteral(".mov")));
}

TEST(ProxyManager, DetectsProxyState)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString proxy =
		QDir(dir.path()).filePath(QStringLiteral("proxy-file.mp4"));
	EXPECT_EQ(olive::ProxyManager::GetProxyState(proxy),
			  olive::ProxyManager::kProxyMissing);

	QFile working(olive::ProxyManager::GetWorkingProxyFilename(proxy));
	ASSERT_TRUE(working.open(QFile::WriteOnly));
	working.close();
	EXPECT_EQ(olive::ProxyManager::GetProxyState(proxy),
			  olive::ProxyManager::kProxyGenerating);

	QFile ready(proxy);
	ASSERT_TRUE(ready.open(QFile::WriteOnly));
	ready.close();
	EXPECT_EQ(olive::ProxyManager::GetProxyState(proxy),
			  olive::ProxyManager::kProxyReady);
}

TEST(ProxyManager, ReadyStateTakesPrecedenceOverWorkingFile)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString proxy =
		QDir(dir.path()).filePath(QStringLiteral("proxy-file.mp4"));
	QFile ready(proxy);
	ASSERT_TRUE(ready.open(QFile::WriteOnly));
	ready.close();

	QFile working(olive::ProxyManager::GetWorkingProxyFilename(proxy));
	ASSERT_TRUE(working.open(QFile::WriteOnly));
	working.close();

	EXPECT_EQ(olive::ProxyManager::GetProxyState(proxy),
			  olive::ProxyManager::kProxyReady);
}

TEST(ProxyManager, ConvertsProxyStateToAndFromStrings)
{
	EXPECT_EQ(olive::ProxyManager::ProxyStateToString(
				  olive::ProxyManager::kProxyMissing),
			  QStringLiteral("missing"));
	EXPECT_EQ(olive::ProxyManager::ProxyStateToString(
				  olive::ProxyManager::kProxyGenerating),
			  QStringLiteral("generating"));
	EXPECT_EQ(olive::ProxyManager::ProxyStateToString(
				  olive::ProxyManager::kProxyReady),
			  QStringLiteral("ready"));
	EXPECT_EQ(olive::ProxyManager::ProxyStateToString(
				  olive::ProxyManager::kProxyFailed),
			  QStringLiteral("failed"));

	EXPECT_EQ(
		olive::ProxyManager::ProxyStateFromString(QStringLiteral("missing")),
		olive::ProxyManager::kProxyMissing);
	EXPECT_EQ(
		olive::ProxyManager::ProxyStateFromString(QStringLiteral("generating")),
		olive::ProxyManager::kProxyGenerating);
	EXPECT_EQ(
		olive::ProxyManager::ProxyStateFromString(QStringLiteral("ready")),
		olive::ProxyManager::kProxyReady);
	EXPECT_EQ(
		olive::ProxyManager::ProxyStateFromString(QStringLiteral("failed")),
		olive::ProxyManager::kProxyFailed);
	EXPECT_EQ(
		olive::ProxyManager::ProxyStateFromString(QStringLiteral("unknown")),
		olive::ProxyManager::kProxyMissing);
}

TEST(ProxyManager, FootagePersistsProxyMetadata)
{
	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	writer.writeTextElement(QStringLiteral("timestamp"), QStringLiteral("0"));
	writer.writeStartElement(QStringLiteral("proxy"));
	writer.writeAttribute(QStringLiteral("enabled"), QStringLiteral("1"));
	writer.writeAttribute(QStringLiteral("state"), QStringLiteral("ready"));
	writer.writeAttribute(QStringLiteral("stream"), QStringLiteral("0"));
	writer.writeAttribute(QStringLiteral("preset"), QStringLiteral("1"));
	writer.writeCharacters(QStringLiteral("/cache/proxy/example.mp4"));
	writer.writeEndElement();
	writer.writeEndElement();
	writer.writeEndDocument();

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));

	olive::Footage footage;
	ASSERT_TRUE(footage.LoadCustom(&reader, nullptr));
	EXPECT_TRUE(footage.proxy_enabled());
	EXPECT_EQ(footage.proxy_path(), QStringLiteral("/cache/proxy/example.mp4"));
	EXPECT_EQ(footage.proxy_state(), olive::ProxyManager::kProxyReady);
	EXPECT_EQ(footage.proxy_video_stream_index(), 0);
	EXPECT_EQ(footage.proxy_preset_version(), 1);
}

TEST(ProxyManager, FootageSavesProxyMetadata)
{
	olive::Footage footage;
	footage.set_timestamp(42);
	footage.SetProxy(QStringLiteral("/cache/proxy/example.mp4"),
					 olive::ProxyManager::kProxyReady, 2, 3, true);

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	footage.SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	EXPECT_TRUE(xml.contains(QStringLiteral("<proxy")));
	EXPECT_TRUE(xml.contains(QStringLiteral("enabled=\"1\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("state=\"ready\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("stream=\"2\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("preset=\"3\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("/cache/proxy/example.mp4")));
}

TEST(ProxyManager, FootageClearRemovesProxyMetadata)
{
	olive::Footage footage;
	footage.SetProxy(QStringLiteral("/cache/proxy/example.mp4"),
					 olive::ProxyManager::kProxyReady, 0, 1, true);

	footage.Clear();

	EXPECT_FALSE(footage.proxy_enabled());
	EXPECT_TRUE(footage.proxy_path().isEmpty());
	EXPECT_EQ(footage.proxy_state(), olive::ProxyManager::kProxyMissing);
	EXPECT_EQ(footage.proxy_video_stream_index(), -1);
	EXPECT_EQ(footage.proxy_preset_version(), 0);
}

TEST(ProxyManager, EmitsProxyFinishedState)
{
	olive::ProxyManager::CreateInstance();

	bool received = false;
	QString received_source;
	int received_stream = -1;
	QString received_proxy;
	olive::ProxyManager::ProxyState received_state =
		olive::ProxyManager::kProxyMissing;
	QObject::connect(
		olive::ProxyManager::instance(), &olive::ProxyManager::ProxyFinished,
		[&received, &received_source, &received_stream, &received_proxy,
		 &received_state](const QString &source_filename, int stream_index,
						  const QString &proxy_filename,
						  olive::ProxyManager::ProxyState state) {
			received = true;
			received_source = source_filename;
			received_stream = stream_index;
			received_proxy = proxy_filename;
			received_state = state;
		});

	emit olive::ProxyManager::instance()
		-> ProxyFinished(QStringLiteral("/media/source.mov"), 0,
						 QStringLiteral("/cache/proxy/example.mp4"),
						 olive::ProxyManager::kProxyFailed);

	EXPECT_TRUE(received);
	EXPECT_EQ(received_source, QStringLiteral("/media/source.mov"));
	EXPECT_EQ(received_stream, 0);
	EXPECT_EQ(received_proxy, QStringLiteral("/cache/proxy/example.mp4"));
	EXPECT_EQ(received_state, olive::ProxyManager::kProxyFailed);

	olive::ProxyManager::DestroyInstance();
}

TEST(ProxyManager, WorkingProxyFilenamePrependsExtension)
{
	const QString proxy = QStringLiteral("/cache/proxy/example.mp4");
	const QString working = olive::ProxyManager::GetWorkingProxyFilename(proxy);

	EXPECT_EQ(working, QStringLiteral("/cache/proxy/example.mp4.working.mp4"));
}

TEST(ProxyManager, ProxyStateFromStringDefaultsForEmpty)
{
	EXPECT_EQ(olive::ProxyManager::ProxyStateFromString(QString()),
			  olive::ProxyManager::kProxyMissing);
}

TEST(ProxyManager, FootageProxyCanBeDisabled)
{
	olive::Footage footage;
	footage.SetProxy(QStringLiteral("/cache/proxy/example.mp4"),
					 olive::ProxyManager::kProxyReady, 0, 1, false);

	EXPECT_FALSE(footage.proxy_enabled());
	EXPECT_FALSE(footage.proxy_path().isEmpty());
}

TEST(ProxyManager, FootageJobWithoutProxyHasEmptyProxyFields)
{
	olive::FootageJob job(olive::TimeRange(), QStringLiteral("source-decoder"),
						  QStringLiteral("/media/source.mov"),
						  olive::Track::kVideo, olive::rational(10),
						  olive::LoopMode::kLoopModeOff);

	EXPECT_FALSE(job.has_proxy());
	EXPECT_TRUE(job.proxy_filename().isEmpty());
	EXPECT_TRUE(job.proxy_decoder().isEmpty());
	EXPECT_EQ(job.proxy_stream_index(), -1);
}

TEST(ProxyManager, ProxyFilenameIncludesAudioFlag)
{
	olive::ProxyManager::ProxyParams params;
	params.include_audio = true;

	const QString with_audio = olive::ProxyManager::GetProxyFilename(
		QStringLiteral("/tmp/oak-cache"), QStringLiteral("/media/source.mov"),
		0, params);
	EXPECT_TRUE(with_audio.contains(QStringLiteral(".a1.")));
	EXPECT_TRUE(olive::ProxyManager::ProxyFilenameHasAudio(with_audio));

	params.include_audio = false;
	const QString without_audio = olive::ProxyManager::GetProxyFilename(
		QStringLiteral("/tmp/oak-cache"), QStringLiteral("/media/source.mov"),
		0, params);
	EXPECT_TRUE(without_audio.contains(QStringLiteral(".a0.")));
	EXPECT_FALSE(olive::ProxyManager::ProxyFilenameHasAudio(without_audio));

	// Legacy proxy filenames (no audio marker) must be treated as video-only
	EXPECT_FALSE(olive::ProxyManager::ProxyFilenameHasAudio(
		QStringLiteral("/tmp/oak-cache/proxy/abc-0.1280x720.v1.mp4")));

	// The audio flag distinguishes otherwise identical proxy filenames
	EXPECT_NE(with_audio, without_audio);
}

TEST(ProxyManager, ProxyParamsFromConfigReadsDefaults)
{
	const olive::ProxyManager::ProxyParams params =
		olive::ProxyManager::ProxyParamsFromConfig();

	EXPECT_EQ(params.width, ProxyConfigValue("ProxyWidth").value<int>());
	EXPECT_EQ(params.height, ProxyConfigValue("ProxyHeight").value<int>());
	EXPECT_EQ(params.crf, ProxyConfigValue("ProxyCRF").value<int>());
	EXPECT_EQ(params.preset, ProxyConfigValue("ProxyPreset").toString());
	EXPECT_EQ(params.include_audio,
			  ProxyConfigValue("ProxyIncludeAudio").toBool());
}

TEST(ProxyManager, FindFFmpegExecutablePrefersConfiguredPath)
{
	// The test executable itself is guaranteed to be an existing executable
	// file, making it a safe stand-in for an ffmpeg binary
	const QString self = QCoreApplication::applicationFilePath();
	ASSERT_FALSE(self.isEmpty());

	EXPECT_EQ(olive::ProxyManager::FindFFmpegExecutable(self), self);
}

TEST(ProxyManager, FindFFmpegExecutableRejectsInvalidConfiguredPath)
{
	const QString bogus = QStringLiteral("/nonexistent/ffmpeg-binary");
	const QString result = olive::ProxyManager::FindFFmpegExecutable(bogus);

	// Must not return the invalid configured path; any fallback is acceptable
	EXPECT_NE(result, bogus);
}

TEST(ProxyTask, BuildArgumentsIncludesAudioWhenEnabled)
{
	olive::ProxyManager::ProxyParams params;
	params.include_audio = true;

	const QStringList args = olive::ProxyTask::BuildArguments(
		QStringLiteral("/media/source.mov"), 1, params,
		QStringLiteral("/cache/proxy/out.mp4"));

	EXPECT_FALSE(args.contains(QStringLiteral("-an")));
	const int audio_map = args.indexOf(QStringLiteral("0:a?"));
	EXPECT_GE(audio_map, 0);
	EXPECT_GT(audio_map, args.indexOf(QStringLiteral("-map")));
	EXPECT_TRUE(args.contains(QStringLiteral("-c:a")));
	EXPECT_TRUE(args.contains(QStringLiteral("aac")));
	// The requested video stream must be mapped before the audio streams
	EXPECT_LT(args.indexOf(QStringLiteral("0:1")), audio_map);
}

TEST(ProxyTask, BuildArgumentsDisablesAudioWhenDisabled)
{
	olive::ProxyManager::ProxyParams params;
	params.include_audio = false;

	const QStringList args = olive::ProxyTask::BuildArguments(
		QStringLiteral("/media/source.mov"), 1, params,
		QStringLiteral("/cache/proxy/out.mp4"));

	EXPECT_TRUE(args.contains(QStringLiteral("-an")));
	EXPECT_FALSE(args.contains(QStringLiteral("0:a?")));
}

TEST(ProxyManager, FootagePersistsCustomProxyParams)
{
	olive::Footage footage;
	olive::ProxyManager::ProxyParams params;
	params.width = 640;
	params.height = 360;
	params.crf = 30;
	params.preset = QStringLiteral("faster");
	params.extension = QStringLiteral("mov");
	params.include_audio = false;
	footage.SetCustomProxyParams(params);
	footage.SetProxy(QStringLiteral("/cache/proxy/example.mp4"),
					 olive::ProxyManager::kProxyReady, 0, 1, true);

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	footage.SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	EXPECT_TRUE(xml.contains(QStringLiteral("custom=\"1\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("pwidth=\"640\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("pheight=\"360\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("pcrf=\"30\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("ppreset=\"faster\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("pext=\"mov\"")));
	EXPECT_TRUE(xml.contains(QStringLiteral("paudio=\"0\"")));

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));

	olive::Footage loaded;
	ASSERT_TRUE(loaded.LoadCustom(&reader, nullptr));
	ASSERT_TRUE(loaded.has_custom_proxy_params());
	EXPECT_EQ(loaded.custom_proxy_params().width, 640);
	EXPECT_EQ(loaded.custom_proxy_params().height, 360);
	EXPECT_EQ(loaded.custom_proxy_params().crf, 30);
	EXPECT_EQ(loaded.custom_proxy_params().preset, QStringLiteral("faster"));
	EXPECT_EQ(loaded.custom_proxy_params().extension, QStringLiteral("mov"));
	EXPECT_FALSE(loaded.custom_proxy_params().include_audio);
}

TEST(ProxyManager, FootageWithoutCustomParamsOmitsThemFromXml)
{
	olive::Footage footage;
	footage.SetProxy(QStringLiteral("/cache/proxy/example.mp4"),
					 olive::ProxyManager::kProxyReady, 0, 1, true);

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	footage.SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	EXPECT_FALSE(xml.contains(QStringLiteral("custom=")));

	// Loading old project files without custom params must not enable them
	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	olive::Footage loaded;
	ASSERT_TRUE(loaded.LoadCustom(&reader, nullptr));
	EXPECT_FALSE(loaded.has_custom_proxy_params());
}

TEST(ProxyManager, FootageEffectiveProxyParams)
{
	olive::Footage footage;

	// Without custom params, the global config values apply
	const olive::ProxyManager::ProxyParams global_params =
		footage.GetEffectiveProxyParams();
	EXPECT_EQ(global_params.width, ProxyConfigValue("ProxyWidth").value<int>());
	EXPECT_EQ(global_params.include_audio,
			  ProxyConfigValue("ProxyIncludeAudio").toBool());

	// Custom params take precedence
	olive::ProxyManager::ProxyParams custom;
	custom.width = 320;
	custom.height = 180;
	footage.SetCustomProxyParams(custom);
	EXPECT_TRUE(footage.has_custom_proxy_params());
	EXPECT_EQ(footage.GetEffectiveProxyParams().width, 320);
	EXPECT_EQ(footage.GetEffectiveProxyParams().height, 180);

	// Clearing reverts to the global config values
	footage.ClearCustomProxyParams();
	EXPECT_FALSE(footage.has_custom_proxy_params());
	EXPECT_EQ(footage.GetEffectiveProxyParams().width,
			  ProxyConfigValue("ProxyWidth").value<int>());
}
