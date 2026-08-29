#include <gtest/gtest.h>

#include "config/config.h"
#include "render/rendermanager.h"

TEST(Config, DefaultsPresent)
{
	olive::Config &cfg = olive::Config::Current();
	cfg.SetDefaults();

	EXPECT_TRUE(cfg[QStringLiteral("Style")].isValid());
	EXPECT_TRUE(cfg[QStringLiteral("TimecodeDisplay")].isValid());
	EXPECT_TRUE(cfg[QStringLiteral("DefaultStillLength")].isValid());
	EXPECT_EQ(cfg[QStringLiteral("GraphicsBackend")].toString(),
			  QStringLiteral("opengl"));
}

TEST(Config, SetAndGetValues)
{
	olive::Config &cfg = olive::Config::Current();
	cfg[QStringLiteral("UnitTestValue")] = 42;
	EXPECT_EQ(cfg[QStringLiteral("UnitTestValue")].toInt(), 42);

	cfg[QStringLiteral("UnitTestString")] = QStringLiteral("hello");
	EXPECT_EQ(cfg[QStringLiteral("UnitTestString")].toString(),
			  QStringLiteral("hello"));

	cfg[QStringLiteral("UnitTestBool")] = true;
	EXPECT_TRUE(cfg[QStringLiteral("UnitTestBool")].toBool());

	cfg[QStringLiteral("UnitTestDouble")] = 3.14;
	EXPECT_NEAR(cfg[QStringLiteral("UnitTestDouble")].toDouble(), 3.14, 0.001);
}

TEST(Config, MissingKeyReturnsInvalidVariant)
{
	olive::Config &cfg = olive::Config::Current();
	EXPECT_FALSE(cfg[QStringLiteral("DefinitelyMissingKey")].isValid());
}

TEST(Config, GraphicsBackendStringConversion)
{
	EXPECT_EQ(olive::RenderManager::BackendFromString(QStringLiteral("opengl")),
			  olive::RenderManager::kOpenGL);
	EXPECT_EQ(olive::RenderManager::BackendFromString(QStringLiteral("vulkan")),
			  olive::RenderManager::kVulkan);
	EXPECT_EQ(olive::RenderManager::BackendFromString(QStringLiteral("dummy")),
			  olive::RenderManager::kDummy);
	EXPECT_EQ(
		olive::RenderManager::BackendFromString(QStringLiteral("multiprocess")),
		olive::RenderManager::kMultiProcess);
	EXPECT_EQ(
		olive::RenderManager::BackendToString(olive::RenderManager::kOpenGL),
		QStringLiteral("opengl"));
	EXPECT_EQ(
		olive::RenderManager::BackendToString(olive::RenderManager::kVulkan),
		QStringLiteral("vulkan"));
	EXPECT_EQ(
		olive::RenderManager::BackendToString(olive::RenderManager::kDummy),
		QStringLiteral("dummy"));
	EXPECT_EQ(olive::RenderManager::BackendToString(
				  olive::RenderManager::kMultiProcess),
			  QStringLiteral("multiprocess"));
	EXPECT_EQ(olive::RenderManager::BackendFromString(QStringLiteral("bad")),
			  olive::RenderManager::kOpenGL);
}

TEST(Config, SetDefaultsPopulatesRequiredKeys)
{
	olive::Config &cfg = olive::Config::Current();
	cfg.SetDefaults();

	const QStringList required = {
		QStringLiteral("Style"),
		QStringLiteral("TimecodeDisplay"),
		QStringLiteral("DefaultStillLength"),
		QStringLiteral("GraphicsBackend"),
		QStringLiteral("AutoCacheDelay"),
		QStringLiteral("AudioOutput"),
		QStringLiteral("AudioInput"),
	};

	foreach (const QString &key, required) {
		EXPECT_TRUE(cfg[key].isValid()) << key.toStdString();
	}
}
