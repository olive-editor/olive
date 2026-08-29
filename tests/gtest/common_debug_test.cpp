#include <gtest/gtest.h>

#include "common/debug.h"

TEST(CommonDebug, DebugHandlerFormatsAllLevels)
{
	// Install handler and restore after test
	QtMessageHandler old = qInstallMessageHandler(olive::DebugHandler);

	qDebug() << "debug message";
	qInfo() << "info message";
	qWarning() << "warning message";
	qCritical() << "critical message";

	qInstallMessageHandler(old);
}
