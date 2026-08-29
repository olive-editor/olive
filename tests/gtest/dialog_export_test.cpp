#include <gtest/gtest.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QSignalSpy>
#include <QStandardPaths>

#include "codec/encoder.h"
#include "dialog/export/codec/h264section.h"
#include "dialog/export/codec/imagesection.h"
#include "dialog/export/exportadvancedvideodialog.h"
#include "dialog/export/exportaudiotab.h"
#include "dialog/export/exportformatcombobox.h"
#include "dialog/export/exportsavepresetdialog.h"
#include "dialog/export/exportsubtitlestab.h"
#include "dialog/export/exportvideotab.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"

namespace
{

// Redirects QStandardPaths (used for the export preset directory) to a
// disposable test location for the lifetime of the guard
class StandardPathsTestModeGuard {
public:
	StandardPathsTestModeGuard()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	~StandardPathsTestModeGuard()
	{
		QStandardPaths::setTestModeEnabled(false);
	}
};

QList<int> MenuFormatData(const olive::ExportFormatComboBox &combo)
{
	QList<int> formats;
	// custom_menu_ is an olive::Menu (a QMenu without its own Q_OBJECT)
	auto *menu = combo.findChild<QMenu *>();
	if (!menu) {
		return formats;
	}
	foreach (QAction *a, menu->actions()) {
		if (!a->isSeparator() && a->data().isValid()) {
			formats.append(a->data().toInt());
		}
	}
	return formats;
}

} // namespace

//
// export: format combobox
//
TEST(DialogExportFormatComboBox, GetSetFormatRoundTrip)
{
	olive::ExportFormatComboBox combo;

	// Before any selection the format is the invalid placeholder
	EXPECT_EQ(combo.GetFormat(), olive::ExportFormat::kFormatCount);

	combo.SetFormat(olive::ExportFormat::kFormatMatroska);
	EXPECT_EQ(combo.GetFormat(), olive::ExportFormat::kFormatMatroska);
	EXPECT_EQ(combo.currentText(),
			  olive::ExportFormat::GetName(olive::ExportFormat::kFormatMatroska));
}

TEST(DialogExportFormatComboBox, MenuSelectionEmitsFormatChanged)
{
	olive::ExportFormatComboBox combo;
	QSignalSpy spy(&combo, &olive::ExportFormatComboBox::FormatChanged);

	QAction action(QStringLiteral("QuickTime"), &combo);
	action.setData(static_cast<int>(olive::ExportFormat::kFormatQuickTime));

	QMetaObject::invokeMethod(&combo, "HandleIndexChange",
							  Q_ARG(QAction *, &action));

	EXPECT_EQ(combo.GetFormat(), olive::ExportFormat::kFormatQuickTime);
	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().first().toInt(),
			  static_cast<int>(olive::ExportFormat::kFormatQuickTime));
}

TEST(DialogExportFormatComboBox, AudioOnlyModeListsOnlyAudioFormats)
{
	olive::ExportFormatComboBox combo(
		olive::ExportFormatComboBox::kShowAudioOnly);

	const QList<int> formats = MenuFormatData(combo);
	EXPECT_FALSE(formats.isEmpty());

	foreach (int f, formats) {
		const auto fmt = static_cast<olive::ExportFormat::Format>(f);
		EXPECT_TRUE(olive::ExportFormat::GetVideoCodecs(fmt).isEmpty())
			<< "Format " << f << " should not have video codecs";
		EXPECT_FALSE(olive::ExportFormat::GetAudioCodecs(fmt).isEmpty())
			<< "Format " << f << " should have audio codecs";
	}

	EXPECT_TRUE(formats.contains(
		static_cast<int>(olive::ExportFormat::kFormatWAV)));
	EXPECT_FALSE(formats.contains(
		static_cast<int>(olive::ExportFormat::kFormatMatroska)));
}

//
// export: audio tab
//
TEST(DialogExportAudioTab, SetFormatPopulatesCodecs)
{
	olive::ExportAudioTab tab;

	const QList<olive::ExportCodec::Codec> codecs =
		olive::ExportFormat::GetAudioCodecs(olive::ExportFormat::kFormatMatroska);
	ASSERT_FALSE(codecs.isEmpty());

	EXPECT_EQ(tab.SetFormat(olive::ExportFormat::kFormatMatroska),
			  codecs.size());

	// The first codec is auto-selected
	EXPECT_EQ(tab.GetCodec(), codecs.first());
}

TEST(DialogExportAudioTab, LosslessCodecDisablesBitRate)
{
	olive::ExportAudioTab tab;
	tab.SetFormat(olive::ExportFormat::kFormatMatroska);

	tab.SetCodec(olive::ExportCodec::kCodecAAC);
	EXPECT_TRUE(tab.bit_rate_slider()->isEnabled());
	EXPECT_EQ(tab.GetCodec(), olive::ExportCodec::kCodecAAC);

	// PCM is lossless, so no bit rate setting applies
	tab.SetCodec(olive::ExportCodec::kCodecPCM);
	EXPECT_EQ(tab.GetCodec(), olive::ExportCodec::kCodecPCM);
	EXPECT_FALSE(tab.bit_rate_slider()->isEnabled());
	EXPECT_TRUE(tab.bit_rate_slider()->IsTristate());
}

TEST(DialogExportAudioTab, FormatWithoutAudioCodecsDisablesTab)
{
	olive::ExportAudioTab tab;

	// PNG carries no audio
	EXPECT_EQ(tab.SetFormat(olive::ExportFormat::kFormatPNG), 0);
	EXPECT_FALSE(tab.isEnabled());
}

//
// export: subtitles tab
//
TEST(DialogExportSubtitlesTab, SidecarStateFollowsFormatCapabilities)
{
	olive::ExportSubtitlesTab tab;
	tab.SetSidecarFormat(olive::ExportFormat::kFormatSRT);

	auto *sidecar_box = tab.findChild<QCheckBox *>();
	ASSERT_NE(sidecar_box, nullptr);

	// Matroska can embed subtitles: sidecar is optional and off by default
	tab.SetFormat(olive::ExportFormat::kFormatMatroska);
	EXPECT_TRUE(sidecar_box->isEnabled());
	EXPECT_FALSE(tab.GetSidecarEnabled());
	EXPECT_EQ(tab.GetSubtitleCodec(), olive::ExportCodec::kCodecSRT);

	// SetSidecarEnabled toggles the check state (used to restore params)
	tab.SetSidecarEnabled(true);
	EXPECT_TRUE(tab.GetSidecarEnabled());
	tab.SetSidecarEnabled(false);
	EXPECT_FALSE(tab.GetSidecarEnabled());

	// SRT is a subtitles-only format: sidecar makes no sense, forced off
	tab.SetFormat(olive::ExportFormat::kFormatSRT);
	EXPECT_FALSE(sidecar_box->isEnabled());
	EXPECT_FALSE(tab.GetSidecarEnabled());

	// WAV cannot carry subtitles at all: sidecar is forced on
	tab.SetFormat(olive::ExportFormat::kFormatWAV);
	EXPECT_FALSE(sidecar_box->isEnabled());
	EXPECT_TRUE(tab.GetSidecarEnabled());
}

//
// export: video tab
//
TEST(DialogExportVideoTab, SetFormatPopulatesCodecs)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ExportVideoTab tab(project.color_manager());

	const QList<olive::ExportCodec::Codec> codecs =
		olive::ExportFormat::GetVideoCodecs(olive::ExportFormat::kFormatMatroska);
	ASSERT_FALSE(codecs.isEmpty());

	EXPECT_EQ(tab.SetFormat(olive::ExportFormat::kFormatMatroska),
			  codecs.size());
	EXPECT_EQ(tab.GetSelectedCodec(), codecs.first());

	tab.SetSelectedCodec(olive::ExportCodec::kCodecH265);
	EXPECT_EQ(tab.GetSelectedCodec(), olive::ExportCodec::kCodecH265);
}

TEST(DialogExportVideoTab, CodecSelectsMatchingSection)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ExportVideoTab tab(project.color_manager());
	tab.SetFormat(olive::ExportFormat::kFormatMatroska);

	// First Matroska codec is H.264, which has a dedicated section
	tab.VideoCodecChanged();
	EXPECT_NE(tab.GetCodecSection(), nullptr);

	// Still image codecs get the image section instead
	tab.SetFormat(olive::ExportFormat::kFormatPNG);
	tab.VideoCodecChanged();
	EXPECT_NE(dynamic_cast<olive::ImageSection *>(tab.GetCodecSection()),
			  nullptr);
}

TEST(DialogExportVideoTab, ImageSequenceCheckboxRoundTrips)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ExportVideoTab tab(project.color_manager());
	tab.SetFormat(olive::ExportFormat::kFormatPNG);
	tab.VideoCodecChanged();

	tab.SetImageSequence(true);
	EXPECT_TRUE(tab.IsImageSequenceSet());

	tab.SetImageSequence(false);
	EXPECT_FALSE(tab.IsImageSequenceSet());
}

TEST(DialogExportVideoTab, MaintainAspectTogglesScalingMethod)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ExportVideoTab tab(project.color_manager());

	tab.maintain_aspect_checkbox()->setChecked(true);
	EXPECT_FALSE(tab.scaling_method_combobox()->isEnabled());

	tab.maintain_aspect_checkbox()->setChecked(false);
	EXPECT_TRUE(tab.scaling_method_combobox()->isEnabled());
}

//
// export: codec sections
//
TEST(DialogExportH264CRFSection, ValueRoundTripsAndClamps)
{
	olive::H264CRFSection section(olive::H264CRFSection::kDefaultH264CRF);

	EXPECT_EQ(section.GetValue(), olive::H264CRFSection::kDefaultH264CRF);

	section.SetValue(30);
	EXPECT_EQ(section.GetValue(), 30);

	section.SetValue(99);
	EXPECT_EQ(section.GetValue(), 51);

	section.SetValue(-5);
	EXPECT_EQ(section.GetValue(), 0);
}

TEST(DialogExportH264BitRateSection, BitRateRoundTripsInBits)
{
	olive::H264BitRateSection section;

	section.SetTargetBitRate(8000000);
	EXPECT_EQ(section.GetTargetBitRate(), 8000000);

	section.SetMaximumBitRate(16000000);
	EXPECT_EQ(section.GetMaximumBitRate(), 16000000);
}

//
// export: advanced video dialog
//
TEST(DialogExportAdvancedVideo, FieldsRoundTrip)
{
	olive::ExportAdvancedVideoDialog dialog({ QStringLiteral("yuv420p"),
											  QStringLiteral("yuv422p") });

	dialog.set_threads(4);
	EXPECT_EQ(dialog.threads(), 4);

	dialog.set_pix_fmt(QStringLiteral("yuv422p"));
	EXPECT_EQ(dialog.pix_fmt(), QStringLiteral("yuv422p"));

	dialog.set_yuv_range(olive::VideoParams::kColorRangeFull);
	EXPECT_EQ(dialog.yuv_range(), olive::VideoParams::kColorRangeFull);
}

//
// export: save preset dialog
//
TEST(DialogExportSavePreset, AcceptWritesPresetFile)
{
	StandardPathsTestModeGuard test_mode;

	olive::EncodingParams params;

	olive::ExportSavePresetDialog dialog(params);

	auto *name_edit = dialog.findChild<QLineEdit *>();
	ASSERT_NE(name_edit, nullptr);
	name_edit->setText(QStringLiteral("oak-test-preset"));

	EXPECT_EQ(dialog.GetSelectedPresetName(),
			  QStringLiteral("oak-test-preset"));

	dialog.accept();
	EXPECT_EQ(dialog.result(), QDialog::Accepted);

	EXPECT_TRUE(olive::EncodingParams::GetListOfPresets().contains(
		QStringLiteral("oak-test-preset")));

	// Clean up the preset file written to the test config location
	QFile::remove(QDir(olive::EncodingParams::GetPresetPath())
					  .filePath(QStringLiteral("oak-test-preset")));
}
