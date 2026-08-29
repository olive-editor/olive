#include <gtest/gtest.h>

#include "config/config.h"
#include "dialog/proxy/proxydialog.h"
#include "node/project/footage/footage.h"

namespace
{

QVariant ProxyDialogConfigValue(const char *key)
{
	return olive::Config::Current()[QString::fromUtf8(key)];
}

} // namespace

TEST(ProxyDialog, ConstructsInGlobalModeWithNullParent)
{
	olive::ProxyDialog dialog(nullptr);

	// The global settings editors must reflect the current config values
	EXPECT_EQ(dialog.ProxyWidth(),
			  ProxyDialogConfigValue("ProxyWidth").value<int>());
	EXPECT_EQ(dialog.ProxyHeight(),
			  ProxyDialogConfigValue("ProxyHeight").value<int>());
	EXPECT_EQ(dialog.ProxyCRF(),
			  ProxyDialogConfigValue("ProxyCRF").value<int>());
	EXPECT_EQ(dialog.ProxyPreset(),
			  ProxyDialogConfigValue("ProxyPreset").toString());
	EXPECT_EQ(dialog.ProxyIncludeAudio(),
			  ProxyDialogConfigValue("ProxyIncludeAudio").toBool());
	EXPECT_EQ(dialog.FFmpegPath(),
			  ProxyDialogConfigValue("FFmpegPath").toString());
}

TEST(ProxyDialog, ConstructsWithFootageList)
{
	olive::Footage footage;
	const QVector<olive::Footage *> items = { &footage };

	olive::ProxyDialog dialog(nullptr, items);
	SUCCEED();
}

TEST(ProxyDialog, AcceptSavesGlobalSettingsToConfig)
{
	const int old_width = ProxyDialogConfigValue("ProxyWidth").value<int>();
	const bool old_include_audio =
		ProxyDialogConfigValue("ProxyIncludeAudio").toBool();
	const QString old_ffmpeg_path =
		ProxyDialogConfigValue("FFmpegPath").toString();

	{
		olive::ProxyDialog dialog(nullptr);
		dialog.SetProxyWidth(640);
		dialog.SetProxyIncludeAudio(!old_include_audio);
		dialog.SetFFmpegPath(QStringLiteral("/tmp/oak-test-ffmpeg"));
		dialog.accept();
	}

	EXPECT_EQ(ProxyDialogConfigValue("ProxyWidth").value<int>(), 640);
	EXPECT_EQ(ProxyDialogConfigValue("ProxyIncludeAudio").toBool(),
			  !old_include_audio);
	EXPECT_EQ(ProxyDialogConfigValue("FFmpegPath").toString(),
			  QStringLiteral("/tmp/oak-test-ffmpeg"));

	// Restore previous config values so other tests are unaffected
	olive::Config::Current()[QStringLiteral("ProxyWidth")] = old_width;
	olive::Config::Current()[QStringLiteral("ProxyIncludeAudio")] =
		old_include_audio;
	olive::Config::Current()[QStringLiteral("FFmpegPath")] = old_ffmpeg_path;
}
