#include <gtest/gtest.h>

#include "cli/cliprogress/cliprogressdialog.h"
#include "cli/clitask/clitaskdialog.h"
#include "task/task.h"

namespace
{

class DummyTask : public olive::Task {
public:
	explicit DummyTask(bool succeed)
		: succeed_(succeed)
	{
		SetTitle(QStringLiteral("Dummy"));
	}

protected:
	virtual bool Run() override
	{
		emit ProgressChanged(0.5);
		emit ProgressChanged(1.0);
		return succeed_;
	}

private:
	bool succeed_;
};

} // namespace

TEST(CLIProgress, ConstructorPrintsTitleAndZeroPercent)
{
	testing::internal::CaptureStdout();
	{
		olive::CLIProgressDialog dlg(QStringLiteral("Exporting"));
	}
	const QString out =
		QString::fromStdString(testing::internal::GetCapturedStdout());

	EXPECT_TRUE(out.contains(QStringLiteral("Exporting")));
	EXPECT_TRUE(out.contains(QStringLiteral("0%")));
}

TEST(CLIProgress, SameProgressValueDoesNotRedraw)
{
	olive::CLIProgressDialog dlg(QStringLiteral("Job"));

	testing::internal::CaptureStdout();
	dlg.SetProgress(0.0);
	const QString out =
		QString::fromStdString(testing::internal::GetCapturedStdout());

	EXPECT_TRUE(out.isEmpty());
}

TEST(CLIProgress, ProgressRendersPercentage)
{
	olive::CLIProgressDialog dlg(QStringLiteral("Job"));

	testing::internal::CaptureStdout();
	dlg.SetProgress(0.25);
	const QString out =
		QString::fromStdString(testing::internal::GetCapturedStdout());

	EXPECT_TRUE(out.contains(QStringLiteral("25%")));
}

TEST(CLIProgress, LongTitleIsTruncatedWithEllipsis)
{
	const QString long_title(80, QLatin1Char('a'));

	testing::internal::CaptureStdout();
	{
		olive::CLIProgressDialog dlg(long_title);
	}
	const QString out =
		QString::fromStdString(testing::internal::GetCapturedStdout());

	EXPECT_TRUE(out.contains(QStringLiteral("...")));
	EXPECT_FALSE(out.contains(long_title));
}

TEST(CLIProgress, BarFillMatchesProgress)
{
	olive::CLIProgressDialog dlg(QStringLiteral("Job"));

	testing::internal::CaptureStdout();
	dlg.SetProgress(1.0);
	const QString out =
		QString::fromStdString(testing::internal::GetCapturedStdout());

	// 80-column layout: bar area is 80/2-7 = 33 columns, all filled at 100%.
	EXPECT_EQ(out.count(QLatin1Char('=')), 33);
}

TEST(CLIProgress, PercentageIsPaddedToThreeColumns)
{
	olive::CLIProgressDialog dlg(QStringLiteral("Job"));

	auto pct_field = [&](double p) {
		testing::internal::CaptureStdout();
		dlg.SetProgress(p);
		const QString out =
			QString::fromStdString(testing::internal::GetCapturedStdout());
		const int bracket = out.indexOf(QLatin1Char(']'));
		const int percent = out.indexOf(QLatin1Char('%'));
		EXPECT_GE(bracket, 0);
		EXPECT_GT(percent, bracket);
		return out.mid(bracket + 1, percent - bracket - 1);
	};

	// Single, double and triple digit percentages should all occupy the same
	// field width so the bar stays aligned while progressing.
	EXPECT_EQ(pct_field(0.05), QStringLiteral("   5"));
	EXPECT_EQ(pct_field(0.5), QStringLiteral("  50"));
	EXPECT_EQ(pct_field(1.0), QStringLiteral(" 100"));
}

TEST(CLITask, RunReturnsTaskResult)
{
	{
		DummyTask task(true);
		olive::CLITaskDialog dlg(&task);

		testing::internal::CaptureStdout();
		const bool ok = dlg.Run();
		testing::internal::GetCapturedStdout();

		EXPECT_TRUE(ok);
	}

	{
		DummyTask task(false);
		olive::CLITaskDialog dlg(&task);

		testing::internal::CaptureStdout();
		const bool ok = dlg.Run();
		testing::internal::GetCapturedStdout();

		EXPECT_FALSE(ok);
	}
}

TEST(CLITask, TaskProgressIsForwardedToDisplay)
{
	DummyTask task(true);
	olive::CLITaskDialog dlg(&task);

	testing::internal::CaptureStdout();
	dlg.Run();
	const QString out =
		QString::fromStdString(testing::internal::GetCapturedStdout());

	EXPECT_TRUE(out.contains(QStringLiteral("Dummy")));
	EXPECT_TRUE(out.contains(QStringLiteral("50%")));
	EXPECT_TRUE(out.contains(QStringLiteral("100%")));
}
