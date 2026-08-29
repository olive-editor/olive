#include <gtest/gtest.h>

#include <QSignalSpy>

#include "node/factory.h"
#include "render/videoparams.h"
#include "ui/colorcoding.h"
#include "widget/colorlabelmenu/colorcodingcombobox.h"
#include "widget/nodecombobox/nodecombobox.h"
#include "widget/standardcombos/standardcombos.h"

TEST(WidgetCombos, SampleRateContainsSupportedRates)
{
	olive::SampleRateComboBox combo;

	EXPECT_EQ(combo.count(),
			  int(olive::AudioParams::kSupportedSampleRates.size()));

	for (int rate : olive::AudioParams::kSupportedSampleRates) {
		combo.SetSampleRate(rate);
		EXPECT_EQ(combo.GetSampleRate(), rate);
	}
}

TEST(WidgetCombos, ChannelLayoutRoundTrips)
{
	olive::ChannelLayoutComboBox combo;

	EXPECT_EQ(combo.count(),
			  int(olive::AudioParams::kSupportedChannelLayouts.size()));

	for (uint64_t layout : olive::AudioParams::kSupportedChannelLayouts) {
		combo.SetChannelLayout(layout);
		EXPECT_EQ(combo.GetChannelLayout(), layout);
	}
}

TEST(WidgetCombos, InterlacedIndexesMatchEnum)
{
	olive::InterlacedComboBox combo;

	ASSERT_EQ(combo.count(), 3);

	combo.SetInterlaceMode(olive::VideoParams::kInterlaceNone);
	EXPECT_EQ(combo.GetInterlaceMode(), olive::VideoParams::kInterlaceNone);
	EXPECT_EQ(combo.currentIndex(), int(olive::VideoParams::kInterlaceNone));

	combo.SetInterlaceMode(olive::VideoParams::kInterlacedTopFirst);
	EXPECT_EQ(combo.GetInterlaceMode(), olive::VideoParams::kInterlacedTopFirst);
	EXPECT_EQ(combo.currentIndex(), int(olive::VideoParams::kInterlacedTopFirst));

	combo.SetInterlaceMode(olive::VideoParams::kInterlacedBottomFirst);
	EXPECT_EQ(combo.GetInterlaceMode(),
			  olive::VideoParams::kInterlacedBottomFirst);
	EXPECT_EQ(combo.currentIndex(),
			  int(olive::VideoParams::kInterlacedBottomFirst));
}

TEST(WidgetCombos, PixelFormatAllFormatsPresent)
{
	olive::PixelFormatComboBox combo(false);

	EXPECT_EQ(combo.count(), int(olive::core::PixelFormat::COUNT));

	combo.SetPixelFormat(olive::core::PixelFormat::F32);
	EXPECT_EQ(static_cast<olive::core::PixelFormat::Format>(combo.GetPixelFormat()),
			  olive::core::PixelFormat::F32);

	combo.SetPixelFormat(olive::core::PixelFormat::U8);
	EXPECT_EQ(static_cast<olive::core::PixelFormat::Format>(combo.GetPixelFormat()),
			  olive::core::PixelFormat::U8);
}

TEST(WidgetCombos, PixelFormatFloatOnlyFilters)
{
	olive::PixelFormatComboBox combo(true);

	EXPECT_GT(combo.count(), 0);
	EXPECT_LT(combo.count(), int(olive::core::PixelFormat::COUNT));

	for (int i = 0; i < combo.count(); i++) {
		olive::core::PixelFormat fmt =
			static_cast<olive::core::PixelFormat::Format>(
				combo.itemData(i).toInt());
		EXPECT_TRUE(fmt.is_float());
	}
}

TEST(WidgetCombos, VideoDividerRoundTrips)
{
	olive::VideoDividerComboBox combo;

	EXPECT_EQ(combo.count(), olive::VideoParams::kSupportedDividers.size());

	for (int d : olive::VideoParams::kSupportedDividers) {
		combo.SetDivider(d);
		EXPECT_EQ(combo.GetDivider(), d);
	}
}

TEST(WidgetCombos, FrameRateStandardAndCustom)
{
	olive::FrameRateComboBox combo;

	// Defaults to the first standard rate
	EXPECT_EQ(combo.GetFrameRate(),
			  olive::VideoParams::kSupportedFrameRates.first());

	// Selecting a standard rate just looks it up in the list
	const olive::rational standard =
		olive::VideoParams::kSupportedFrameRates.at(2);
	combo.SetFrameRate(standard);
	EXPECT_EQ(combo.GetFrameRate(), standard);

	// A non-standard rate becomes the custom entry
	const olive::rational custom(27, 2);
	combo.SetFrameRate(custom);
	EXPECT_EQ(combo.GetFrameRate(), custom);

	// Switching back to a standard rate works again
	combo.SetFrameRate(standard);
	EXPECT_EQ(combo.GetFrameRate(), standard);
}

TEST(WidgetCombos, PixelAspectRatioStandardAndCustom)
{
	olive::PixelAspectRatioComboBox combo;

	const QVector<olive::rational> &standards =
		olive::VideoParams::kStandardPixelAspects;
	ASSERT_GE(standards.size(), 2);

	combo.SetPixelAspectRatio(standards.at(1));
	EXPECT_EQ(combo.GetPixelAspectRatio(), standards.at(1));

	// An unknown ratio lands on the last "Custom" item
	const olive::rational custom(17, 13);
	combo.SetPixelAspectRatio(custom);
	EXPECT_EQ(combo.GetPixelAspectRatio(), custom);
	EXPECT_EQ(combo.currentIndex(), combo.count() - 1);
}

TEST(WidgetCombos, SampleFormatPackedFormatsRoundTrip)
{
	using Format = olive::core::SampleFormat::Format;

	olive::SampleFormatComboBox combo;
	combo.SetPackedFormats();

	EXPECT_EQ(combo.count(),
			  int(olive::core::SampleFormat::PACKED_END) -
				  int(olive::core::SampleFormat::PACKED_START));

	for (int i = olive::core::SampleFormat::PACKED_START;
		 i < olive::core::SampleFormat::PACKED_END; i++) {
		const Format fmt = static_cast<Format>(i);
		combo.SetSampleFormat(fmt);
		EXPECT_EQ(static_cast<Format>(combo.GetSampleFormat()), fmt);
	}

	// Re-populating with restore enabled (the default) keeps the selection
	combo.SetSampleFormat(olive::core::SampleFormat::F32);
	combo.SetPackedFormats();
	EXPECT_EQ(static_cast<Format>(combo.GetSampleFormat()),
			  olive::core::SampleFormat::F32);

	// Requesting a format that isn't in the list leaves the selection alone
	combo.SetSampleFormat(olive::core::SampleFormat::F32P);
	EXPECT_EQ(static_cast<Format>(combo.GetSampleFormat()),
			  olive::core::SampleFormat::F32);
}

TEST(WidgetCombos, NodeComboBoxTracksSelectionWithoutSignal)
{
	olive::NodeFactory::Initialize();

	{
		olive::NodeComboBox combo;
		QSignalSpy spy(&combo, &olive::NodeComboBox::NodeChanged);

		const QString id = QStringLiteral("org.olivevideoeditor.Olive.math");
		combo.SetNode(id);
		EXPECT_EQ(combo.GetSelectedNode(), id);
		EXPECT_EQ(combo.count(), 1);
		EXPECT_EQ(combo.itemText(0), olive::NodeFactory::GetNameFromID(id));
		EXPECT_FALSE(combo.itemText(0).isEmpty());

		// Programmatic SetNode never emits NodeChanged
		EXPECT_EQ(spy.count(), 0);

		// Setting the same ID again is a no-op
		combo.SetNode(id);
		EXPECT_EQ(combo.count(), 1);
		EXPECT_EQ(spy.count(), 0);

		// Clearing the selection empties the list
		combo.SetNode(QString());
		EXPECT_TRUE(combo.GetSelectedNode().isEmpty());
		EXPECT_EQ(combo.count(), 0);
	}

	olive::NodeFactory::Destroy();
}

TEST(WidgetCombos, ColorCodingComboSetColor)
{
	olive::ColorCodingComboBox combo;

	EXPECT_EQ(combo.GetSelectedColor(), 0);
	EXPECT_EQ(combo.count(), 1);
	EXPECT_EQ(combo.itemText(0), olive::ColorCoding::GetColorName(0));

	combo.SetColor(3);
	EXPECT_EQ(combo.GetSelectedColor(), 3);
	EXPECT_EQ(combo.count(), 1);
	EXPECT_EQ(combo.itemText(0), olive::ColorCoding::GetColorName(3));
}
