#include <gtest/gtest.h>

#include <cmath>

#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>

#include "common/ratiodialog.h"

using namespace olive;

namespace
{

// Drives the modal dialogs that GetFloatRatioFromUser() pops up: feeds the
// queued text responses into QInputDialogs and accepts any QMessageBox shown
// in between (i.e. the invalid-ratio warning)
class DialogDriver : public QObject {
public:
	explicit DialogDriver(const QStringList &responses)
		: responses_(responses)
	{
		connect(&timer_, &QTimer::timeout, this, &DialogDriver::Step);
		timer_.start(10);
	}

private:
	void Step()
	{
		QWidget *modal = QApplication::activeModalWidget();
		if (!modal) {
			return;
		}

		if (auto *box = qobject_cast<QMessageBox *>(modal)) {
			box->accept();
			return;
		}

		if (auto *input = qobject_cast<QInputDialog *>(modal)) {
			if (responses_.isEmpty()) {
				// No more responses queued: cancel the dialog
				input->reject();
				timer_.stop();
				return;
			}

			input->setTextValue(responses_.takeFirst());
			input->accept();

			if (responses_.isEmpty()) {
				timer_.stop();
			}
		}
	}

	QStringList responses_;
	QTimer timer_;
};

} // namespace

TEST(CommonRatioDialog, AcceptsPlainDecimal)
{
	DialogDriver driver({ QStringLiteral("1.5") });

	bool ok = false;
	const double ratio = GetFloatRatioFromUser(nullptr, QStringLiteral("Test"), &ok);

	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(ratio, 1.5);
}

TEST(CommonRatioDialog, AcceptsColonSeparatedRatio)
{
	DialogDriver driver({ QStringLiteral("16:9") });

	bool ok = false;
	const double ratio = GetFloatRatioFromUser(nullptr, QStringLiteral("Test"), &ok);

	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(ratio, 16.0 / 9.0);
}

TEST(CommonRatioDialog, AcceptsSlashSeparatedRatio)
{
	DialogDriver driver({ QStringLiteral("4/3") });

	bool ok = false;
	const double ratio = GetFloatRatioFromUser(nullptr, QStringLiteral("Test"), &ok);

	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(ratio, 4.0 / 3.0);
}

TEST(CommonRatioDialog, AcceptsSemicolonSeparatedRatio)
{
	DialogDriver driver({ QStringLiteral("1;2") });

	bool ok = false;
	const double ratio = GetFloatRatioFromUser(nullptr, QStringLiteral("Test"), &ok);

	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(ratio, 0.5);
}

TEST(CommonRatioDialog, CancelReturnsNaN)
{
	// An empty response list makes the driver cancel the input dialog
	DialogDriver driver({});

	bool ok = true;
	const double ratio = GetFloatRatioFromUser(nullptr, QStringLiteral("Test"), &ok);

	EXPECT_FALSE(ok);
	EXPECT_TRUE(std::isnan(ratio));
}

TEST(CommonRatioDialog, InvalidInputWarnsAndRetries)
{
	// "banana" fails to parse (the driver accepts the warning box), the retry
	// with a valid ratio succeeds
	DialogDriver driver({ QStringLiteral("banana"), QStringLiteral("2") });

	bool ok = false;
	const double ratio = GetFloatRatioFromUser(nullptr, QStringLiteral("Test"), &ok);

	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(ratio, 2.0);
}

TEST(CommonRatioDialog, RejectsNonPositiveValues)
{
	// Zero and negative values are rejected with a warning before a valid
	// value is accepted
	DialogDriver driver(
		{ QStringLiteral("0"), QStringLiteral("-4:2"), QStringLiteral("3") });

	bool ok = false;
	const double ratio = GetFloatRatioFromUser(nullptr, QStringLiteral("Test"), &ok);

	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(ratio, 3.0);
}
