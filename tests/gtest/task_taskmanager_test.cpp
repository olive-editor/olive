#include <gtest/gtest.h>

#include <QEventLoop>
#include <QTimer>

#include "task/taskmanager.h"

namespace
{
class DummyTask final : public olive::Task {
public:
	explicit DummyTask(bool *ran)
		: ran_(ran)
	{
		SetTitle(QStringLiteral("DummyTask"));
	}

protected:
	bool Run() override
	{
		if (ran_) {
			*ran_ = true;
		}
		return true;
	}

private:
	bool *ran_ = nullptr;
};

class FailingTask final : public olive::Task {
public:
	FailingTask()
	{
		SetTitle(QStringLiteral("FailingTask"));
	}

protected:
	bool Run() override
	{
		SetError(QStringLiteral("expected failure"));
		return false;
	}
};

class ProgressTask final : public olive::Task {
public:
	explicit ProgressTask(int steps)
		: steps_(steps)
	{
		SetTitle(QStringLiteral("ProgressTask"));
	}

protected:
	bool Run() override
	{
		for (int i = 0; i <= steps_; ++i) {
			emit ProgressChanged(static_cast<double>(i) / steps_);
		}
		return true;
	}

private:
	int steps_ = 1;
};
}

TEST(TaskManager, AddAndRunTask)
{
	olive::TaskManager::CreateInstance();
	olive::TaskManager *mgr = olive::TaskManager::instance();
	ASSERT_NE(mgr, nullptr);

	bool ran = false;
	DummyTask *task = new DummyTask(&ran);

	QEventLoop loop;
	QObject::connect(task, &olive::Task::Finished, &loop,
					 [&loop](olive::Task *, bool) { loop.quit(); });

	mgr->AddTask(task);

	QTimer::singleShot(5000, &loop, &QEventLoop::quit);
	loop.exec();

	EXPECT_TRUE(ran);
	olive::TaskManager::DestroyInstance();
}

TEST(TaskManager, FailedTaskEmitsTaskFailed)
{
	olive::TaskManager::CreateInstance();
	olive::TaskManager *mgr = olive::TaskManager::instance();
	ASSERT_NE(mgr, nullptr);

	FailingTask *task = new FailingTask();

	QEventLoop loop;
	bool saw_failed = false;
	QObject::connect(mgr, &olive::TaskManager::TaskFailed, &loop,
					 [&loop, &saw_failed](olive::Task *) {
						 saw_failed = true;
						 loop.quit();
					 });
	QTimer::singleShot(5000, &loop, &QEventLoop::quit);

	mgr->AddTask(task);
	loop.exec();

	EXPECT_TRUE(saw_failed);
	EXPECT_FALSE(task->GetError().isEmpty());
	olive::TaskManager::DestroyInstance();
}

TEST(TaskManager, ProgressSignalIsEmitted)
{
	olive::TaskManager::CreateInstance();
	olive::TaskManager *mgr = olive::TaskManager::instance();
	ASSERT_NE(mgr, nullptr);

	ProgressTask *task = new ProgressTask(4);

	QEventLoop loop;
	QVector<double> progress;
	QObject::connect(task, &olive::Task::ProgressChanged, &loop,
					 [&progress](double p) { progress.append(p); });
	QObject::connect(task, &olive::Task::Finished, &loop,
					 [&loop](olive::Task *, bool) { loop.quit(); });
	QTimer::singleShot(5000, &loop, &QEventLoop::quit);

	mgr->AddTask(task);
	loop.exec();

	EXPECT_FALSE(progress.isEmpty());
	EXPECT_GE(progress.last(), 0.99);
	olive::TaskManager::DestroyInstance();
}

TEST(TaskManager, MultipleTasksComplete)
{
	olive::TaskManager::CreateInstance();
	olive::TaskManager *mgr = olive::TaskManager::instance();
	ASSERT_NE(mgr, nullptr);

	bool ran1 = false;
	bool ran2 = false;
	DummyTask *task1 = new DummyTask(&ran1);
	DummyTask *task2 = new DummyTask(&ran2);

	QEventLoop loop;
	int finished = 0;
	auto on_finished = [&loop, &finished](olive::Task *, bool) {
		if (++finished == 2) {
			loop.quit();
		}
	};
	QObject::connect(task1, &olive::Task::Finished, &loop, on_finished);
	QObject::connect(task2, &olive::Task::Finished, &loop, on_finished);
	QTimer::singleShot(5000, &loop, &QEventLoop::quit);

	mgr->AddTask(task1);
	mgr->AddTask(task2);
	loop.exec();

	EXPECT_TRUE(ran1);
	EXPECT_TRUE(ran2);
	olive::TaskManager::DestroyInstance();
}
