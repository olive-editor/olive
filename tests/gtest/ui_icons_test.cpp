#include <gtest/gtest.h>

#include <QFile>
#include <QIcon>
#include <QVector>

#include "ui/icons/icons.h"

TEST(UIIcons, ThemeResourcesAreRegistered)
{
	// Both themes compile their icons into the binary via AUTORCC resources
	EXPECT_TRUE(
		QFile::exists(QStringLiteral(":/style/olive-dark/png/play.16.png")));
	EXPECT_TRUE(
		QFile::exists(QStringLiteral(":/style/olive-light/png/play.16.png")));
	EXPECT_TRUE(QFile::exists(QStringLiteral(":/style/olive-dark/palette.ini")));
	EXPECT_TRUE(QFile::exists(QStringLiteral(":/style/olive-dark/style.css")));
}

TEST(UIIcons, CreateLoadsAllSizes)
{
	QIcon icon = olive::icon::Create(QStringLiteral(":/style/olive-dark"),
									 QStringLiteral("play"));
	ASSERT_FALSE(icon.isNull());

	// genicons.sh generates 16/32/64/128 px variants of every icon
	const QList<QSize> sizes = icon.availableSizes();
	for (int sz : { 16, 32, 64, 128 }) {
		EXPECT_TRUE(sizes.contains(QSize(sz, sz))) << "Missing size " << sz;
	}

	EXPECT_FALSE(icon.pixmap(QSize(32, 32)).isNull());
}

TEST(UIIcons, CreateWithUnknownNameYieldsNoUsableIcon)
{
	QIcon icon = olive::icon::Create(QStringLiteral(":/style/olive-dark"),
									 QStringLiteral("no-such-icon"));

	// QIcon::addFile() differs across Qt builds in whether entries for
	// nonexistent files keep the icon "null". What matters is that no usable
	// pixmap can be produced for an unknown icon name.
	EXPECT_TRUE(icon.availableSizes().isEmpty());
	EXPECT_TRUE(icon.pixmap(QSize(16, 16)).isNull());
	EXPECT_TRUE(icon.pixmap(32, 32, QIcon::Disabled).isNull());
}

TEST(UIIcons, LoadAllPopulatesGlobalIcons)
{
	olive::icon::LoadAll(QStringLiteral(":/style/olive-dark"));

	const QVector<QIcon *> all = { &olive::icon::GoToStart,
								   &olive::icon::PrevFrame,
								   &olive::icon::Play,
								   &olive::icon::Pause,
								   &olive::icon::NextFrame,
								   &olive::icon::GoToEnd,
								   &olive::icon::New,
								   &olive::icon::Open,
								   &olive::icon::Save,
								   &olive::icon::Undo,
								   &olive::icon::Redo,
								   &olive::icon::TreeView,
								   &olive::icon::ListView,
								   &olive::icon::IconView,
								   &olive::icon::ToolPointer,
								   &olive::icon::ToolEdit,
								   &olive::icon::ToolRipple,
								   &olive::icon::ToolRolling,
								   &olive::icon::ToolRazor,
								   &olive::icon::ToolSlip,
								   &olive::icon::ToolSlide,
								   &olive::icon::ToolHand,
								   &olive::icon::ToolTransition,
								   &olive::icon::ToolTrackSelect,
								   &olive::icon::Folder,
								   &olive::icon::Sequence,
								   &olive::icon::Video,
								   &olive::icon::Audio,
								   &olive::icon::Image,
								   &olive::icon::MiniMap,
								   &olive::icon::TriUp,
								   &olive::icon::TriLeft,
								   &olive::icon::TriDown,
								   &olive::icon::TriRight,
								   &olive::icon::TextBold,
								   &olive::icon::TextItalic,
								   &olive::icon::TextUnderline,
								   &olive::icon::TextStrikethrough,
								   &olive::icon::TextSmallCaps,
								   &olive::icon::TextAlignLeft,
								   &olive::icon::TextAlignRight,
								   &olive::icon::TextAlignCenter,
								   &olive::icon::TextAlignJustify,
								   &olive::icon::TextAlignTop,
								   &olive::icon::TextAlignBottom,
								   &olive::icon::TextAlignMiddle,
								   &olive::icon::Snapping,
								   &olive::icon::ZoomIn,
								   &olive::icon::ZoomOut,
								   &olive::icon::Record,
								   &olive::icon::Add,
								   &olive::icon::Error,
								   &olive::icon::DirUp,
								   &olive::icon::Clock,
								   &olive::icon::Diamond,
								   &olive::icon::Plus,
								   &olive::icon::Minus,
								   &olive::icon::AddEffect,
								   &olive::icon::EyeOpened,
								   &olive::icon::EyeClosed,
								   &olive::icon::LockOpened,
								   &olive::icon::LockClosed,
								   &olive::icon::Pencil,
								   &olive::icon::Subtitles,
								   &olive::icon::ColorPicker };

	for (const QIcon *icon : all) {
		EXPECT_FALSE(icon->isNull());
	}
}
