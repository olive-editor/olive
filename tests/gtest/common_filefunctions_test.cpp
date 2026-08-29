#include <gtest/gtest.h>

#include <QTemporaryDir>
#include <QTemporaryFile>

#include "common/filefunctions.h"

TEST(CommonFileFunctions, EnsureFilenameExtension)
{
	EXPECT_EQ(olive::FileFunctions::EnsureFilenameExtension(
				  QStringLiteral("project"), QStringLiteral("ove")),
			  QStringLiteral("project.ove"));
	EXPECT_EQ(olive::FileFunctions::EnsureFilenameExtension(
				  QStringLiteral("project.ove"), QStringLiteral("ove")),
			  QStringLiteral("project.ove"));
	EXPECT_EQ(olive::FileFunctions::EnsureFilenameExtension(
				  QStringLiteral("PROJECT"), QStringLiteral("ove")),
			  QStringLiteral("PROJECT.ove"));
	EXPECT_TRUE(olive::FileFunctions::EnsureFilenameExtension(
					QString(), QStringLiteral("ove"))
					.isEmpty());
	EXPECT_EQ(olive::FileFunctions::EnsureFilenameExtension(
				  QStringLiteral("project"), QString()),
			  QStringLiteral("project"));
}

TEST(CommonFileFunctions, GetSafeTemporaryFilename)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	QString base = dir.filePath(QStringLiteral("test.ove"));
	QString first = olive::FileFunctions::GetSafeTemporaryFilename(base);
	EXPECT_FALSE(QFileInfo::exists(first));
	EXPECT_TRUE(first.contains(QStringLiteral(".tmp0.")));

	QFile f(first);
	f.open(QIODevice::WriteOnly);
	f.close();

	QString second = olive::FileFunctions::GetSafeTemporaryFilename(base);
	EXPECT_NE(first, second);
	EXPECT_TRUE(second.contains(QStringLiteral(".tmp1.")));
}

TEST(CommonFileFunctions, DirectoryIsValid)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	EXPECT_TRUE(
		olive::FileFunctions::DirectoryIsValid(QDir(dir.path()), false));

	QDir nonexistent(dir.filePath(QStringLiteral("subdir/nested")));
	EXPECT_TRUE(olive::FileFunctions::DirectoryIsValid(nonexistent, true));
	EXPECT_TRUE(nonexistent.exists());
}

TEST(CommonFileFunctions, RenameFileAllowOverwrite)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	QString from = dir.filePath(QStringLiteral("from.txt"));
	QString to = dir.filePath(QStringLiteral("to.txt"));

	QFile f(from);
	f.open(QIODevice::WriteOnly);
	f.write("source");
	f.close();

	QFile t(to);
	t.open(QIODevice::WriteOnly);
	t.write("existing");
	t.close();

	EXPECT_TRUE(olive::FileFunctions::RenameFileAllowOverwrite(from, to));
	EXPECT_FALSE(QFileInfo::exists(from));
	QFile result(to);
	result.open(QIODevice::ReadOnly);
	EXPECT_EQ(result.readAll(), QByteArray("source"));
}

TEST(CommonFileFunctions, CanCopyDirectoryWithoutOverwriting)
{
	QTemporaryDir src;
	QTemporaryDir dst;
	ASSERT_TRUE(src.isValid());
	ASSERT_TRUE(dst.isValid());

	QString src_file = QDir(src.path()).filePath(QStringLiteral("file.txt"));
	QFile f(src_file);
	f.open(QIODevice::WriteOnly);
	f.close();

	EXPECT_TRUE(olive::FileFunctions::CanCopyDirectoryWithoutOverwriting(
		src.path(), dst.path()));

	QString dst_file = QDir(dst.path()).filePath(QStringLiteral("file.txt"));
	QFile g(dst_file);
	g.open(QIODevice::WriteOnly);
	g.close();

	EXPECT_FALSE(olive::FileFunctions::CanCopyDirectoryWithoutOverwriting(
		src.path(), dst.path()));
}

TEST(CommonFileFunctions, CopyDirectory)
{
	QTemporaryDir src;
	QTemporaryDir dst;
	ASSERT_TRUE(src.isValid());
	ASSERT_TRUE(dst.isValid());

	QString src_file = QDir(src.path()).filePath(QStringLiteral("file.txt"));
	QFile f(src_file);
	f.open(QIODevice::WriteOnly);
	f.write("copied");
	f.close();

	QString dst_dir = QDir(dst.path()).filePath(QStringLiteral("copied"));
	olive::FileFunctions::CopyDirectory(src.path(), dst_dir, false);

	QFile result(QDir(dst_dir).filePath(QStringLiteral("file.txt")));
	EXPECT_TRUE(result.open(QIODevice::ReadOnly));
	EXPECT_EQ(result.readAll(), QByteArray("copied"));
}

TEST(CommonFileFunctions, ReadFileAsString)
{
	QTemporaryFile f;
	ASSERT_TRUE(f.open());
	f.write("hello world");
	f.close();

	EXPECT_EQ(olive::FileFunctions::ReadFileAsString(f.fileName()),
			  QStringLiteral("hello world"));
	EXPECT_TRUE(olive::FileFunctions::ReadFileAsString(
					QStringLiteral("/nonexistent/path"))
					.isEmpty());
}

TEST(CommonFileFunctions, GetUniqueFileIdentifier)
{
	QTemporaryFile f;
	ASSERT_TRUE(f.open());
	f.close();

	QString id1 = olive::FileFunctions::GetUniqueFileIdentifier(f.fileName());
	QString id2 = olive::FileFunctions::GetUniqueFileIdentifier(f.fileName());
	EXPECT_FALSE(id1.isEmpty());
	EXPECT_EQ(id1, id2);

	EXPECT_TRUE(olive::FileFunctions::GetUniqueFileIdentifier(
					QStringLiteral("/nonexistent"))
					.isEmpty());
}

TEST(CommonFileFunctions, GetConfigurationLocation)
{
	QString loc = olive::FileFunctions::GetConfigurationLocation();
	EXPECT_FALSE(loc.isEmpty());
	EXPECT_TRUE(QDir(loc).exists());
}

TEST(CommonFileFunctions, GetTempFilePath)
{
	QString temp = olive::FileFunctions::GetTempFilePath();
	EXPECT_FALSE(temp.isEmpty());
	EXPECT_TRUE(QDir(temp).exists());
}

TEST(CommonFileFunctions, GetAutoRecoveryRoot)
{
	QString root = olive::FileFunctions::GetAutoRecoveryRoot();
	EXPECT_FALSE(root.isEmpty());
}

TEST(CommonFileFunctions, DirectoryIsValidExisting)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	EXPECT_TRUE(
		olive::FileFunctions::DirectoryIsValid(QDir(dir.path()), false));
}

TEST(CommonFileFunctions, CopyDirectoryWithOverwrite)
{
	QTemporaryDir src;
	QTemporaryDir dst;
	ASSERT_TRUE(src.isValid());
	ASSERT_TRUE(dst.isValid());

	QString src_file = QDir(src.path()).filePath(QStringLiteral("file.txt"));
	QFile f(src_file);
	f.open(QIODevice::WriteOnly);
	f.write("new content");
	f.close();

	QString dst_file = QDir(dst.path()).filePath(QStringLiteral("file.txt"));
	QFile g(dst_file);
	g.open(QIODevice::WriteOnly);
	g.write("old content");
	g.close();

	olive::FileFunctions::CopyDirectory(src.path(), dst.path(), true);

	QFile result(dst_file);
	result.open(QIODevice::ReadOnly);
	EXPECT_EQ(result.readAll(), QByteArray("new content"));
}

TEST(CommonFileFunctions, CopyDirectorySourceMissing)
{
	QTemporaryDir dst;
	ASSERT_TRUE(dst.isValid());

	// Should not crash even if source doesn't exist
	olive::FileFunctions::CopyDirectory(QStringLiteral("/nonexistent/path"),
										dst.path(), false);
}
