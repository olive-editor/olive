#include <gtest/gtest.h>

#include "pluginSupport/OliveHost.h"

TEST(PluginSupport, LoadPluginsEmptyPath)
{
	EXPECT_NO_THROW({ olive::plugin::loadPlugins(QString()); });
}
