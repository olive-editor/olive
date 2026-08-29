#include <gtest/gtest.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/factory.h"
#include "node/input/time/timeinput.h"
#include "node/project.h"
#include "node/project/folder/folder.h"
#include "node/project/footage/footage.h"
#include "node/project/serializer/serializer.h"
#include "render/diskmanager.h"
#include "task/project/import/import.h"
#include "task/project/import/importerrordialog.h"
#include "task/project/load/load.h"
#include "undo/undocommand.h"

namespace
{

QString TestImagePath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/img.png"));
}

} // namespace

class TaskProjectImportTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!olive::Core::instance()) {
			// Leaked intentionally: Core is process-wide (matches footage_probe_test)
			new olive::Core(olive::Core::CoreParams());
		}

		created_disk_manager_ = (olive::DiskManager::instance() == nullptr);
		if (created_disk_manager_) {
			olive::DiskManager::CreateInstance();
		}

		// Sandbox the footage metadata cache so real probes write into the
		// temp dir instead of the user's cache
		old_cache_home_ = qgetenv("XDG_CACHE_HOME");
		had_cache_home_ = qEnvironmentVariableIsSet("XDG_CACHE_HOME");
		qputenv("XDG_CACHE_HOME",
				QDir(temp_dir_.path()).filePath(QStringLiteral("xdg")).toUtf8());
		QDir().mkpath(
			QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

		olive::ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	void TearDown() override
	{
		project_.reset();
		if (created_disk_manager_) {
			olive::DiskManager::DestroyInstance();
		}
		if (had_cache_home_) {
			qputenv("XDG_CACHE_HOME", old_cache_home_);
		} else {
			qunsetenv("XDG_CACHE_HOME");
		}
	}

	QTemporaryDir temp_dir_;
	QByteArray old_cache_home_;
	bool had_cache_home_ = false;
	bool created_disk_manager_ = false;
	std::unique_ptr<olive::Project> project_;
};

TEST_F(TaskProjectImportTest, ImportOfUnprobeableFileCollectsInvalidList)
{
	const QString path =
		QDir(temp_dir_.path()).filePath(QStringLiteral("not_media.txt"));
	{
		QFile file(path);
		ASSERT_TRUE(file.open(QFile::WriteOnly));
		file.write("this is not a media file");
	}

	olive::ProjectImportTask task(project_->root(), { path });
	EXPECT_EQ(task.GetFileCount(), 1);

	double last_progress = -1.0;
	QObject::connect(&task, &olive::Task::ProgressChanged, &task,
					 [&last_progress](double p) { last_progress = p; });

	ASSERT_TRUE(task.Start());

	EXPECT_TRUE(task.HasInvalidFiles());
	ASSERT_EQ(task.GetInvalidFiles().size(), 1);
	EXPECT_EQ(task.GetInvalidFiles().first(), path);
	EXPECT_TRUE(task.GetImportedFootage().isEmpty());
	EXPECT_DOUBLE_EQ(last_progress, 1.0);

	// The undo command exists but contains no children since nothing was added
	ASSERT_NE(task.GetCommand(), nullptr);
	EXPECT_EQ(task.GetCommand()->child_count(), 0);
	delete task.GetCommand();
}

TEST_F(TaskProjectImportTest, ImportOfImageFileAddsFootageThroughUndoCommand)
{
	const QString path = TestImagePath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::ProjectImportTask task(project_->root(), { path });
	EXPECT_EQ(task.GetFileCount(), 1);

	ASSERT_TRUE(task.Start());

	EXPECT_FALSE(task.HasInvalidFiles());
	ASSERT_EQ(task.GetImportedFootage().size(), 1);

	olive::Footage *footage = task.GetImportedFootage().first();
	EXPECT_EQ(footage->filename(), path);
	EXPECT_EQ(footage->GetLabel(), QStringLiteral("img.png"));
	EXPECT_TRUE(footage->IsValid());

	// Nothing is in the folder until the command is redone
	EXPECT_TRUE(project_->root()->children().isEmpty());

	ASSERT_NE(task.GetCommand(), nullptr);
	task.GetCommand()->redo_now();

	ASSERT_EQ(project_->root()->children().size(), 1);
	EXPECT_EQ(project_->root()->children().first(),
			  static_cast<olive::Node *>(footage));
	EXPECT_TRUE(project_->nodes().contains(footage));

	task.GetCommand()->undo_now();
	EXPECT_TRUE(project_->root()->children().isEmpty());

	delete task.GetCommand();
}

TEST_F(TaskProjectImportTest, ImportOfDirectoryCreatesFolderHierarchy)
{
	const QString src = TestImagePath();
	ASSERT_TRUE(QFileInfo::exists(src));

	const QString dir_path =
		QDir(temp_dir_.path()).filePath(QStringLiteral("media"));
	const QString sub_path = QDir(dir_path).filePath(QStringLiteral("sub"));
	ASSERT_TRUE(QDir().mkpath(sub_path));

	const QString first = QDir(dir_path).filePath(QStringLiteral("a.png"));
	const QString second = QDir(sub_path).filePath(QStringLiteral("b.png"));
	ASSERT_TRUE(QFile::copy(src, first));
	ASSERT_TRUE(QFile::copy(src, second));

	olive::ProjectImportTask task(project_->root(), { dir_path });
	EXPECT_EQ(task.GetFileCount(), 2);

	ASSERT_TRUE(task.Start());
	EXPECT_FALSE(task.HasInvalidFiles());
	EXPECT_EQ(task.GetImportedFootage().size(), 2);

	ASSERT_NE(task.GetCommand(), nullptr);
	task.GetCommand()->redo_now();

	// Importing a directory creates a folder named after it under the target
	const QVector<olive::Node *> &root_children = project_->root()->children();
	ASSERT_EQ(root_children.size(), 1);
	auto *top_folder = dynamic_cast<olive::Folder *>(root_children.first());
	ASSERT_NE(top_folder, nullptr);
	EXPECT_EQ(top_folder->GetLabel(), QStringLiteral("media"));

	// ...which holds the first image plus a subfolder with the second image
	QVector<olive::Footage *> all_footage =
		top_folder->ListChildrenOfType<olive::Footage>();
	EXPECT_EQ(all_footage.size(), 2);

	QVector<olive::Folder *> sub_folders =
		top_folder->ListChildrenOfType<olive::Folder>();
	ASSERT_EQ(sub_folders.size(), 1);
	EXPECT_EQ(sub_folders.first()->GetLabel(), QStringLiteral("sub"));

	task.GetCommand()->undo_now();
	EXPECT_TRUE(project_->root()->children().isEmpty());

	delete task.GetCommand();
}

TEST_F(TaskProjectImportTest, CancelledBeforeRunReturnsFalseAndDropsCommand)
{
	const QString path = TestImagePath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::ProjectImportTask task(project_->root(), { path });
	task.Cancel();

	EXPECT_FALSE(task.Start());
	EXPECT_EQ(task.GetCommand(), nullptr);
	EXPECT_TRUE(task.GetImportedFootage().isEmpty());
	EXPECT_FALSE(task.HasInvalidFiles());
	EXPECT_TRUE(project_->root()->children().isEmpty());
}

TEST(TaskProjectImportErrorDialog, PopulatesListWithFailedFilenames)
{
	const QStringList failed = { QStringLiteral("/tmp/first.xyz"),
								 QStringLiteral("/tmp/second.xyz"),
								 QStringLiteral("/tmp/third.xyz") };

	olive::ProjectImportErrorDialog dialog(failed);

	EXPECT_EQ(dialog.windowTitle(), QStringLiteral("Import Error"));

	auto *list = dialog.findChild<QListWidget *>();
	ASSERT_NE(list, nullptr);
	ASSERT_EQ(list->count(), failed.size());
	for (int i = 0; i < failed.size(); i++) {
		EXPECT_EQ(list->item(i)->text(), failed.at(i));
	}

	auto *buttons = dialog.findChild<QDialogButtonBox *>();
	ASSERT_NE(buttons, nullptr);
	EXPECT_NE(buttons->button(QDialogButtonBox::Ok), nullptr);
}

TEST(TaskProjectImportErrorDialog, OkButtonAcceptsDialog)
{
	olive::ProjectImportErrorDialog dialog(
		{ QStringLiteral("/tmp/first.xyz") });

	auto *buttons = dialog.findChild<QDialogButtonBox *>();
	ASSERT_NE(buttons, nullptr);

	QPushButton *ok = buttons->button(QDialogButtonBox::Ok);
	ASSERT_NE(ok, nullptr);
	ok->click();

	EXPECT_EQ(dialog.result(), QDialog::Accepted);
	EXPECT_TRUE(dialog.isHidden());
}

class TaskProjectLoadTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		created_disk_manager_ = (olive::DiskManager::instance() == nullptr);
		if (created_disk_manager_) {
			olive::DiskManager::CreateInstance();
		}

		olive::ColorManager::SetUpDefaultConfig();
		olive::NodeFactory::Initialize();
		olive::ProjectSerializer::Initialize();
	}

	void TearDown() override
	{
		olive::ProjectSerializer::Destroy();
		if (created_disk_manager_) {
			olive::DiskManager::DestroyInstance();
		}
	}

	QString SaveProjectToTempFile(olive::Project *project,
								  const QString &filename)
	{
		const QString path = QDir(temp_dir_.path()).filePath(filename);
		olive::ProjectSerializer::SaveData data(
			olive::ProjectSerializer::kProject, project, path);
		olive::ProjectSerializer::Result result =
			olive::ProjectSerializer::Save(data, false);
		if (result.code() != olive::ProjectSerializer::kSuccess) {
			return QString();
		}
		return path;
	}

	QTemporaryDir temp_dir_;
	bool created_disk_manager_ = false;
};

TEST_F(TaskProjectLoadTest, LoadingValidProjectSucceeds)
{
	olive::Project project;
	project.Initialize();

	auto *node = new olive::TimeInput();
	node->SetLabel(QStringLiteral("TimeInput"));
	node->setParent(&project);

	const QString path =
		SaveProjectToTempFile(&project, QStringLiteral("project.ove"));
	ASSERT_FALSE(path.isEmpty());
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::ProjectLoadTask task(path);
	EXPECT_EQ(task.GetFilename(), path);
	EXPECT_EQ(task.GetLoadedProject(), nullptr);

	ASSERT_TRUE(task.Start()) << task.GetError().toStdString();

	olive::Project *loaded = task.GetLoadedProject();
	ASSERT_NE(loaded, nullptr);
	// Project::set_filename() stores native separators on Windows
	EXPECT_EQ(QDir::fromNativeSeparators(loaded->filename()),
			  QDir::fromNativeSeparators(path));
	EXPECT_FALSE(loaded->nodes().isEmpty());

	delete loaded;
}

TEST_F(TaskProjectLoadTest, LoadingMissingFileFails)
{
	const QString path =
		QDir(temp_dir_.path()).filePath(QStringLiteral("missing.ove"));

	olive::ProjectLoadTask task(path);

	EXPECT_FALSE(task.Start());
	EXPECT_FALSE(task.GetError().isEmpty());
	EXPECT_EQ(task.GetLoadedProject(), nullptr);
}

TEST_F(TaskProjectLoadTest, LoadingCorruptFileFails)
{
	const QString path =
		QDir(temp_dir_.path()).filePath(QStringLiteral("corrupt.ove"));
	{
		QFile file(path);
		ASSERT_TRUE(file.open(QFile::WriteOnly));
		file.write("this is not a project file");
	}

	olive::ProjectLoadTask task(path);

	EXPECT_FALSE(task.Start());
	EXPECT_FALSE(task.GetError().isEmpty());
	EXPECT_EQ(task.GetLoadedProject(), nullptr);
}
