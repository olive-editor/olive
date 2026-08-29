#include <gtest/gtest.h>

#include "node/project.h"

TEST(NodeProject, DefaultsAfterConstruction)
{
	olive::Project project;

	EXPECT_EQ(project.filename(), QString());
	EXPECT_EQ(project.name(), QStringLiteral("(untitled)"));
	EXPECT_EQ(project.pretty_filename(), QStringLiteral("(untitled)"));
	EXPECT_TRUE(project.is_new());
	EXPECT_FALSE(project.is_modified());
	EXPECT_TRUE(project.has_autorecovery_been_saved());
	EXPECT_FALSE(project.GetUuid().isNull());
	EXPECT_NE(project.color_manager(), nullptr);
	EXPECT_EQ(project.root(), nullptr);
	EXPECT_TRUE(project.nodes().isEmpty());
}

TEST(NodeProject, FilenameAndNameUpdate)
{
	olive::Project project;

	const QString filename = QStringLiteral("test_project.ove");
	project.set_filename(filename);
	EXPECT_EQ(project.filename(), filename);
	EXPECT_EQ(project.name(), QStringLiteral("test_project"));
	EXPECT_EQ(project.pretty_filename(), filename);
	EXPECT_FALSE(project.is_new());
}

TEST(NodeProject, ModifiedState)
{
	olive::Project project;

	project.set_modified(true);
	EXPECT_TRUE(project.is_modified());
	EXPECT_FALSE(project.has_autorecovery_been_saved());

	project.set_modified(false);
	EXPECT_FALSE(project.is_modified());
	EXPECT_TRUE(project.has_autorecovery_been_saved());
}

TEST(NodeProject, SettingsRoundTrip)
{
	olive::Project project;

	project.SetSetting(
		olive::Project::kCacheLocationSettingKey,
		QString::number(olive::Project::kCacheStoreAlongsideProject));
	EXPECT_EQ(project.GetCacheLocationSetting(),
			  olive::Project::kCacheStoreAlongsideProject);

	project.SetCustomCachePath(QStringLiteral("/tmp/cache"));
	EXPECT_EQ(project.GetCustomCachePath(), QStringLiteral("/tmp/cache"));

	project.SetColorConfigFilename(QStringLiteral("config.ocio"));
	EXPECT_EQ(project.GetColorConfigFilename(), QStringLiteral("config.ocio"));

	project.SetDefaultInputColorSpace(QStringLiteral("ACEScg"));
	EXPECT_EQ(project.GetDefaultInputColorSpace(), QStringLiteral("ACEScg"));

	project.SetColorReferenceSpace(QStringLiteral("ACES - ACEScg"));
	EXPECT_EQ(project.GetColorReferenceSpace(),
			  QStringLiteral("ACES - ACEScg"));
}

TEST(NodeProject, InitializeCreatesRoot)
{
	olive::Project project;
	EXPECT_EQ(project.root(), nullptr);

	project.Initialize();
	EXPECT_NE(project.root(), nullptr);
	EXPECT_EQ(project.root()->GetLabel(), QStringLiteral("Root"));
}

TEST(NodeProject, UuidCanBeRegenerated)
{
	olive::Project project;
	const QUuid original = project.GetUuid();

	project.RegenerateUuid();
	EXPECT_FALSE(project.GetUuid().isNull());
	EXPECT_NE(project.GetUuid(), original);
}

TEST(NodeProject, GetProjectFromObject)
{
	olive::Project project;
	olive::ColorManager *cm = project.color_manager();
	EXPECT_EQ(olive::Project::GetProjectFromObject(cm), &project);
}

TEST(NodeProject, SaveProducesXml)
{
	olive::Project project;
	project.Initialize();

	QByteArray xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("project"));
	project.Save(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	EXPECT_FALSE(xml.isEmpty());
	EXPECT_TRUE(xml.contains("uuid"));
}
