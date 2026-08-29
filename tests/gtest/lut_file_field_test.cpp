#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "config/config.h"
#include "render/lutlibrary.h"
#include "widget/filefield/lutfilefield.h"

namespace
{

class LutLibraryConfigGuard {
public:
	LutLibraryConfigGuard()
		: previous_(olive::Config::Current()[QStringLiteral("LUTLibraryPaths")]
						.toString())
	{
	}

	~LutLibraryConfigGuard()
	{
		olive::Config::Current()[QStringLiteral("LUTLibraryPaths")] = previous_;
	}

private:
	QString previous_;
};

QString WriteFile(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return QString();
	}
	file.close();
	return path;
}

} // namespace

TEST(LutFileField, PopulatesComboFromLibrary)
{
	LutLibraryConfigGuard guard;

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString cube =
		WriteFile(QDir(dir.path()).filePath(QStringLiteral("a.cube")));
	const QString three_dl =
		WriteFile(QDir(dir.path()).filePath(QStringLiteral("b.3dl")));
	const QString other =
		WriteFile(QDir(dir.path()).filePath(QStringLiteral("c.txt")));
	ASSERT_FALSE(cube.isEmpty());
	ASSERT_FALSE(three_dl.isEmpty());
	ASSERT_FALSE(other.isEmpty());

	olive::LUTLibrary::SetDirectories({ dir.path() });

	olive::LutFileField field;

	// One "Other" entry plus one entry per supported LUT
	ASSERT_EQ(field.library_combo()->count(), 3);
	EXPECT_TRUE(field.library_combo()->itemData(0).toString().isEmpty());
	EXPECT_GE(field.library_combo()->findData(cube), 1);
	EXPECT_GE(field.library_combo()->findData(three_dl), 1);
	EXPECT_EQ(field.library_combo()->findData(other), -1);
}

TEST(LutFileField, SelectionFollowsFilenameAndEmitsOnPick)
{
	LutLibraryConfigGuard guard;

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString cube =
		WriteFile(QDir(dir.path()).filePath(QStringLiteral("a.cube")));
	ASSERT_FALSE(cube.isEmpty());

	olive::LUTLibrary::SetDirectories({ dir.path() });

	olive::LutFileField field;

	// A path that is not in the library shows the "Other" entry
	field.SetFilename(QStringLiteral("/custom/elsewhere.cube"));
	EXPECT_EQ(field.library_combo()->currentIndex(), 0);

	// A library path selects its entry
	field.SetFilename(cube);
	EXPECT_GT(field.library_combo()->currentIndex(), 0);

	// Picking a library entry updates the filename and emits the change
	// signal so the parameter bridge applies it like any other edit
	field.SetFilename(QString());
	QSignalSpy spy(&field, &olive::FileField::FilenameChanged);
	const int index = field.library_combo()->findData(cube);
	ASSERT_GE(index, 1);
	emit field.library_combo()->activated(index);

	EXPECT_EQ(field.GetFilename(), cube);
	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().first().toString(), cube);
}
