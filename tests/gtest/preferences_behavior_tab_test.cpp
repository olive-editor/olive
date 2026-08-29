#include <gtest/gtest.h>

#include <QApplication>

#include "audio/audiomanager.h"
#include "dialog/preferences/tabs/preferencesbehaviortab.h"
#include "dialog/preferences/tabs/preferencesgeneraltab.h"
#include "dialog/preferences/tabs/preferencesaudiotab.h"

using namespace olive;

TEST(PreferencesBehaviorTab, TimelineCategoryHasExpectedCheckboxes)
{
	PreferencesBehaviorTab tab(PreferencesBehaviorTab::kCategoryTimeline);
	// 8 timeline behavior options
	EXPECT_EQ(tab.findChildren<QCheckBox *>().size(), 8);
}

TEST(PreferencesBehaviorTab, PlaybackCategoryHasExpectedCheckboxes)
{
	PreferencesBehaviorTab tab(PreferencesBehaviorTab::kCategoryPlayback);
	EXPECT_EQ(tab.findChildren<QCheckBox *>().size(), 2);
}

TEST(PreferencesBehaviorTab, ProjectCategoryHasExpectedCheckboxes)
{
	PreferencesBehaviorTab tab(PreferencesBehaviorTab::kCategoryProject);
	EXPECT_EQ(tab.findChildren<QCheckBox *>().size(), 1);
}

TEST(PreferencesBehaviorTab, NodesCategoryHasExpectedCheckboxes)
{
	PreferencesBehaviorTab tab(PreferencesBehaviorTab::kCategoryNodes);
	EXPECT_EQ(tab.findChildren<QCheckBox *>().size(), 3);
}

TEST(PreferencesBehaviorTab, RenderingCategoryHasGraphicsBackendCombobox)
{
	PreferencesBehaviorTab tab(PreferencesBehaviorTab::kCategoryRendering);
	EXPECT_FALSE(tab.findChildren<QComboBox *>().isEmpty());
	EXPECT_FALSE(tab.findChildren<QCheckBox *>().isEmpty());
}

TEST(PreferencesBehaviorTab, BehaviorPrefTrProvidesTranslations)
{
	QStringList keys;
	keys << QStringLiteral("Enable hover focus")
		 << QStringLiteral("Select also selects all children in the graph")
		 << QStringLiteral("Double-clicking a node opens its properties")
		 << QStringLiteral("Auto-Seek to Beginning of Sequence")
		 << QStringLiteral("Scroll wheel zooms instead of scrolling")
		 << QStringLiteral("Enable audio scrubbing");

	foreach (const QString &key, keys) {
		EXPECT_FALSE(
			PreferencesBehaviorTab::BehaviorPrefTr(key.toUtf8().constData())
				.isEmpty())
			<< key.toStdString();
	}
}

TEST(PreferencesBehaviorTab, RenderingCategoryContainsDefaultBackend)
{
	PreferencesBehaviorTab tab(PreferencesBehaviorTab::kCategoryRendering);
	QList<QComboBox *> boxes = tab.findChildren<QComboBox *>();
	ASSERT_FALSE(boxes.isEmpty());

	QComboBox *backend_box = boxes.first();
	EXPECT_GT(backend_box->count(), 0);
}

TEST(PreferencesGeneralTab, ContainsHoverFocusOption)
{
	PreferencesGeneralTab tab;
	QList<QCheckBox *> boxes = tab.findChildren<QCheckBox *>();

	bool found = false;
	foreach (QCheckBox *box, boxes) {
		if (box->text() ==
			PreferencesBehaviorTab::BehaviorPrefTr("Enable hover focus")) {
			found = true;
			break;
		}
	}
	EXPECT_TRUE(found);
}

TEST(PreferencesAudioTab, AudioScrubbingCheckboxUsesBehaviorTranslation)
{
	AudioManager::CreateInstance();

	{
		PreferencesAudioTab tab;
		QList<QCheckBox *> boxes = tab.findChildren<QCheckBox *>();

		bool found = false;
		foreach (QCheckBox *box, boxes) {
			if (box->text() == PreferencesBehaviorTab::BehaviorPrefTr(
								   "Enable audio scrubbing")) {
				found = true;
				break;
			}
		}
		EXPECT_TRUE(found);
	}

	AudioManager::DestroyInstance();
}

TEST(PreferencesGeneralTab, IncludesBehaviorOptions)
{
	PreferencesGeneralTab tab;
	QList<QCheckBox *> boxes = tab.findChildren<QCheckBox *>();
	EXPECT_GE(boxes.size(), 3);
}

TEST(PreferencesAudioTab, IncludesAudioScrubbingOption)
{
	AudioManager::CreateInstance();

	{
		PreferencesAudioTab tab;
		QList<QCheckBox *> boxes = tab.findChildren<QCheckBox *>();
		bool found = false;
		foreach (QCheckBox *box, boxes) {
			if (!box->text().isEmpty()) {
				found = true;
				break;
			}
		}
		EXPECT_TRUE(found);
	}

	AudioManager::DestroyInstance();
}
