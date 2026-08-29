#include <QApplication>
#include <QDir>
#include <QFile>
#include <gtest/gtest.h>

// Configures process-wide Qt/OCIO state before running gtest. Tests default to
// the offscreen QPA plugin so headless or invalid DISPLAY sessions do not abort
// before gtest can report skips/failures.
int main(int argc, char **argv)
{
	Q_INIT_RESOURCE(ocioconf);
	if (qEnvironmentVariableIsEmpty("OCIO")) {
		qputenv("OCIO",
				QFile::encodeName(QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
									  .filePath(QStringLiteral(
										  "app/render/ocioconf/config.ocio"))));
	}
	if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
		qputenv("QT_QPA_PLATFORM", "offscreen");
	}
	QApplication app(argc, argv);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
