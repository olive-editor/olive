#include <gtest/gtest.h>

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>

#include "codec/conformmanager.h"
#include "config/config.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "render/diskmanager.h"
#include "render/rendermanager.h"
#include "task/customcache/customcachetask.h"
#include "task/precache/precachetask.h"

TEST(TaskCustomCache, CancelBeforeRunReturnsImmediately)
{
	olive::CustomCacheTask task(QStringLiteral("Sequence"));

	bool cancelled_emitted = false;
	QObject::connect(&task, &olive::CustomCacheTask::Cancelled, &task,
					 [&cancelled_emitted] { cancelled_emitted = true; });

	task.Cancel();
	EXPECT_TRUE(cancelled_emitted);

	// Run() sees the cancel flag on entry and returns without ever blocking
	EXPECT_TRUE(task.Start());
}

TEST(TaskCustomCache, RunBlocksUntilCancelled)
{
	olive::CustomCacheTask task(QStringLiteral("Sequence"));

	bool run_returned = false;
	bool run_result = false;
	QThread *thread = QThread::create([&] {
		run_result = task.Start();
		run_returned = true;
	});
	thread->start();

	// The task waits on its condition variable until cancelled
	EXPECT_FALSE(thread->wait(200));
	EXPECT_FALSE(run_returned);

	bool cancelled_emitted = false;
	QObject::connect(&task, &olive::CustomCacheTask::Cancelled, &task,
					 [&cancelled_emitted] { cancelled_emitted = true; });

	task.Cancel();

	ASSERT_TRUE(thread->wait(5000));
	EXPECT_TRUE(run_returned);
	EXPECT_TRUE(run_result);
	EXPECT_TRUE(cancelled_emitted);

	delete thread;
}

TEST(TaskCustomCache, FinishWakesRunWithoutEmittingCancelled)
{
	olive::CustomCacheTask task(QStringLiteral("Sequence"));

	QThread *thread = QThread::create([&] { task.Start(); });
	thread->start();

	EXPECT_FALSE(thread->wait(200));

	bool cancelled_emitted = false;
	QObject::connect(&task, &olive::CustomCacheTask::Cancelled, &task,
					 [&cancelled_emitted] { cancelled_emitted = true; });

	task.Finish();

	ASSERT_TRUE(thread->wait(5000));
	EXPECT_FALSE(cancelled_emitted);

	delete thread;
}

class TaskPreCacheTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!olive::Core::instance()) {
			// Leaked intentionally: Core is process-wide (matches footage_probe_test)
			new olive::Core(olive::Core::CoreParams());
		}

		olive::ColorManager::SetUpDefaultConfig();

		// Use the dummy render backend so no GPU is touched (matches
		// preview_autocacher_test)
		olive::Config::Current()[QStringLiteral("GraphicsBackend")] =
			QStringLiteral("dummy");

		created_disk_manager_ = (olive::DiskManager::instance() == nullptr);
		if (created_disk_manager_) {
			olive::DiskManager::CreateInstance();
		}
		created_conform_manager_ = (olive::ConformManager::instance() == nullptr);
		if (created_conform_manager_) {
			olive::ConformManager::CreateInstance();
		}
		created_render_manager_ = (olive::RenderManager::instance() == nullptr);
		if (created_render_manager_) {
			olive::RenderManager::CreateInstance();
		}

		// Sandbox the footage metadata cache so real probes write into the
		// temp dir instead of the user's cache
		old_cache_home_ = qgetenv("XDG_CACHE_HOME");
		had_cache_home_ = qEnvironmentVariableIsSet("XDG_CACHE_HOME");
		qputenv("XDG_CACHE_HOME",
				QDir(temp_dir_.path()).filePath(QStringLiteral("xdg")).toUtf8());
		QDir().mkpath(
			QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	void TearDown() override
	{
		project_.reset();
		if (created_render_manager_) {
			olive::RenderManager::DestroyInstance();
		}
		if (created_conform_manager_) {
			olive::ConformManager::DestroyInstance();
		}
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
	bool created_conform_manager_ = false;
	bool created_render_manager_ = false;
	std::unique_ptr<olive::Project> project_;
};

TEST_F(TaskPreCacheTest, ConstructorCopiesFootageIntoPrivateProject)
{
	const QString path = QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
							 .filePath(QStringLiteral("tests/img.png"));
	ASSERT_TRUE(QFileInfo::exists(path));

	auto *footage = new olive::Footage(path);
	footage->setParent(project_.get());
	ASSERT_TRUE(footage->IsValid());

	auto *sequence = new olive::Sequence();
	sequence->setParent(project_.get());
	sequence->set_default_parameters();

	// Construction copies the footage into a private project and wires it to a
	// private viewer; it must not touch the render pipeline. Run() itself is
	// not exercised here since it requires live render workers.
	olive::PreCacheTask task(footage, 0, sequence);

	EXPECT_TRUE(task.GetTitle().contains(path));
	EXPECT_TRUE(task.GetTitle().contains(QStringLiteral(":0")));
}
