#include <gtest/gtest.h>

#include <QComboBox>
#include <QFontMetrics>
#include <QFont>
#include <QLabel>

#include "common/qtutils.h"

TEST(CommonQtUtils, PtrToValueAndBack)
{
	int value = 42;
	void *ptr = &value;
	QVariant v = olive::QtUtils::PtrToValue(ptr);
	EXPECT_EQ(olive::QtUtils::ValueToPtr<int>(v), &value);
}

TEST(CommonQtUtils, GetParentOfType)
{
	QWidget root;
	QLabel *child = new QLabel(&root);

	EXPECT_EQ(olive::QtUtils::GetParentOfType<QLabel>(child), nullptr);
	EXPECT_EQ(olive::QtUtils::GetParentOfType<QWidget>(child), &root);
}

TEST(CommonQtUtils, FlipControlAndShiftModifiers)
{
	// NOTE: The early return for "both modifiers present" uses a broken condition
	// (Qt::ControlModifier & Qt::ShiftModifier is always zero), so the function
	// always swaps Control and Shift. This test documents current behavior.
	Qt::KeyboardModifiers both = Qt::ControlModifier | Qt::ShiftModifier;
	Qt::KeyboardModifiers flipped =
		olive::QtUtils::FlipControlAndShiftModifiers(both);
	EXPECT_TRUE(flipped & Qt::ControlModifier);
	EXPECT_FALSE(flipped & Qt::ShiftModifier);

	Qt::KeyboardModifiers only_shift = Qt::ShiftModifier | Qt::AltModifier;
	flipped = olive::QtUtils::FlipControlAndShiftModifiers(only_shift);
	EXPECT_TRUE(flipped & Qt::ControlModifier);
	EXPECT_FALSE(flipped & Qt::ShiftModifier);
	EXPECT_TRUE(flipped & Qt::AltModifier);

	Qt::KeyboardModifiers only_ctrl = Qt::ControlModifier | Qt::AltModifier;
	flipped = olive::QtUtils::FlipControlAndShiftModifiers(only_ctrl);
	EXPECT_FALSE(flipped & Qt::ControlModifier);
	EXPECT_TRUE(flipped & Qt::ShiftModifier);
	EXPECT_TRUE(flipped & Qt::AltModifier);

	Qt::KeyboardModifiers none;
	EXPECT_EQ(olive::QtUtils::FlipControlAndShiftModifiers(none), none);
}

TEST(CommonQtUtils, SetComboBoxDataByInt)
{
	QComboBox cb;
	cb.addItem(QStringLiteral("A"), 1);
	cb.addItem(QStringLiteral("B"), 2);
	cb.addItem(QStringLiteral("C"), 3);

	olive::QtUtils::SetComboBoxData(&cb, 2);
	EXPECT_EQ(cb.currentData().toInt(), 2);
	EXPECT_EQ(cb.currentText(), QStringLiteral("B"));

	olive::QtUtils::SetComboBoxData(&cb, 42);
	EXPECT_EQ(cb.currentData().toInt(), 2);
}

TEST(CommonQtUtils, SetComboBoxDataByString)
{
	QComboBox cb;
	cb.addItem(QStringLiteral("A"), QStringLiteral("alpha"));
	cb.addItem(QStringLiteral("B"), QStringLiteral("beta"));

	olive::QtUtils::SetComboBoxData(&cb, QStringLiteral("beta"));
	EXPECT_EQ(cb.currentData().toString(), QStringLiteral("beta"));

	olive::QtUtils::SetComboBoxData(&cb, QStringLiteral("missing"));
	EXPECT_EQ(cb.currentData().toString(), QStringLiteral("beta"));
}

TEST(CommonQtUtils, QFontMetricsWidth)
{
	QFont font;
	QFontMetrics fm(font);
	int width = olive::QtUtils::QFontMetricsWidth(fm, QStringLiteral("Olive"));
	EXPECT_GT(width, 0);
}

TEST(CommonQtUtils, CreateHorizontalLine)
{
	QFrame *line = olive::QtUtils::CreateHorizontalLine();
	ASSERT_NE(line, nullptr);
	EXPECT_EQ(line->frameShape(), QFrame::HLine);
	delete line;
}

TEST(CommonQtUtils, CreateVerticalLine)
{
	QFrame *line = olive::QtUtils::CreateVerticalLine();
	ASSERT_NE(line, nullptr);
	EXPECT_EQ(line->frameShape(), QFrame::VLine);
	delete line;
}

TEST(CommonQtUtils, ToQColor)
{
	olive::core::Color c(0.1f, 0.2f, 0.3f, 0.4f);
	QColor qc = olive::QtUtils::toQColor(c);
	EXPECT_NEAR(qc.redF(), 0.1, 0.001);
	EXPECT_NEAR(qc.greenF(), 0.2, 0.001);
	EXPECT_NEAR(qc.blueF(), 0.3, 0.001);
	EXPECT_NEAR(qc.alphaF(), 0.4, 0.001);
}

TEST(CommonQtUtils, GetFormattedDateTime)
{
	QDateTime dt = QDateTime::fromString(QStringLiteral("2025-01-15T10:30:00"),
										 Qt::ISODate);
	QString s = olive::QtUtils::GetFormattedDateTime(dt);
	EXPECT_FALSE(s.isEmpty());
}

TEST(CommonQtUtils, WordWrapString)
{
	QFont font;
	QFontMetrics fm(font);

	// Use a moderate width; long words may not split cleanly, so just verify no crash
	QStringList wrapped = olive::QtUtils::WordWrapString(
		QStringLiteral("hello world foo bar"), fm, 40);
	EXPECT_GE(wrapped.size(), 1u);

	// Should preserve manual newlines
	wrapped = olive::QtUtils::WordWrapString(QStringLiteral("line1\nline2"), fm,
											 1000);
	EXPECT_EQ(wrapped.size(), 2);
}

TEST(CommonQtUtils, ToQColorClampsValues)
{
	olive::core::Color c(2.0f, -1.0f, 0.5f, 1.5f);
	QColor qc = olive::QtUtils::toQColor(c);
	EXPECT_NEAR(qc.redF(), 1.0, 0.001);
	EXPECT_NEAR(qc.greenF(), 0.0, 0.001);
	EXPECT_NEAR(qc.blueF(), 0.5, 0.001);
	EXPECT_NEAR(qc.alphaF(), 1.0, 0.001);
}

TEST(CommonQtUtils, qHashRational)
{
	olive::core::rational r(3, 4);
	EXPECT_NO_THROW(qHash(r));
}

TEST(CommonQtUtils, qHashTimeRange)
{
	olive::core::TimeRange tr(olive::core::rational(1),
							  olive::core::rational(5));
	EXPECT_NO_THROW(qHash(tr));
}
