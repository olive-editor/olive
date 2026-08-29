#include <gtest/gtest.h>

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyle>

#include "ui/style/style.h"

TEST(UIStyle, InitPopulatesThemesAndAppliesStyle)
{
	olive::StyleManager::Init();

	const QMap<QString, QString> &themes = olive::StyleManager::available_themes();
	EXPECT_EQ(themes.size(), 2);
	EXPECT_EQ(themes.value(QStringLiteral("olive-dark")),
			  QStringLiteral("Oak Dark"));
	EXPECT_EQ(themes.value(QStringLiteral("olive-light")),
			  QStringLiteral("Oak Light"));

	// Whatever the config says, the current style must be a known theme
	EXPECT_TRUE(themes.contains(olive::StyleManager::GetStyle()));
	EXPECT_FALSE(qApp->styleSheet().isEmpty());
}

TEST(UIStyle, SetStyleAppliesPaletteFromIni)
{
	olive::StyleManager::SetStyle(QStringLiteral("olive-dark"));
	EXPECT_EQ(olive::StyleManager::GetStyle(), QStringLiteral("olive-dark"));

	// Values from app/ui/style/olive-dark/palette.ini
	const QPalette p = qApp->palette();
	EXPECT_EQ(p.color(QPalette::Window), QColor(QStringLiteral("#353535")));
	EXPECT_EQ(p.color(QPalette::Base), QColor(QStringLiteral("#191919")));
	EXPECT_EQ(p.color(QPalette::Text), QColor(QStringLiteral("#FFFFFF")));
	EXPECT_EQ(p.color(QPalette::Highlight), QColor(QStringLiteral("#2A82DA")));
	EXPECT_EQ(p.color(QPalette::Disabled, QPalette::Text),
			  QColor(QStringLiteral("#A0A0A0")));
	EXPECT_EQ(p.color(QPalette::Disabled, QPalette::ButtonText),
			  QColor(QStringLiteral("#808080")));

	EXPECT_FALSE(qApp->styleSheet().isEmpty());
}

TEST(UIStyle, SetStyleSwitchesThemes)
{
	olive::StyleManager::SetStyle(QStringLiteral("olive-dark"));
	const QColor dark_window = qApp->palette().color(QPalette::Window);

	olive::StyleManager::SetStyle(QStringLiteral("olive-light"));
	EXPECT_EQ(olive::StyleManager::GetStyle(), QStringLiteral("olive-light"));

	// Values from app/ui/style/olive-light/palette.ini
	EXPECT_EQ(qApp->palette().color(QPalette::Window),
			  QColor(QStringLiteral("#D0D0D0")));
	EXPECT_NE(qApp->palette().color(QPalette::Window), dark_window);

	// Restore the default theme for subsequent tests
	olive::StyleManager::SetStyle(olive::StyleManager::kDefaultStyle);
	EXPECT_EQ(olive::StyleManager::GetStyle(),
			  QStringLiteral("olive-dark"));
}

TEST(UIStyle, SetStyleWithMissingThemeClearsOverrides)
{
	olive::StyleManager::SetStyle(QStringLiteral("does-not-exist"));
	EXPECT_EQ(olive::StyleManager::GetStyle(),
			  QStringLiteral("does-not-exist"));

	// No palette.ini/style.css in this theme: fall back to standard palette
	// and an empty stylesheet
	EXPECT_TRUE(qApp->styleSheet().isEmpty());
	EXPECT_EQ(qApp->palette().color(QPalette::Window),
			  qApp->style()->standardPalette().color(QPalette::Window));

	// Restore the default theme for subsequent tests
	olive::StyleManager::SetStyle(olive::StyleManager::kDefaultStyle);
}
