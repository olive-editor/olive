#include <gtest/gtest.h>

#include <QPushButton>

#include "widget/columnedgridlayout/columnedgridlayout.h"
#include "widget/flowlayout/flowlayout.h"

TEST(WidgetLayout, FlowLayoutCountsAndTakesItems)
{
	QWidget container;
	FlowLayout *layout = new FlowLayout(&container, 0, 0, 0);

	auto *a = new QPushButton(QStringLiteral("A"));
	auto *b = new QPushButton(QStringLiteral("B"));
	layout->addWidget(a);
	layout->addWidget(b);

	ASSERT_EQ(layout->count(), 2);
	EXPECT_EQ(layout->itemAt(0)->widget(), a);
	EXPECT_EQ(layout->itemAt(1)->widget(), b);
	EXPECT_EQ(layout->itemAt(2), nullptr);

	QLayoutItem *taken = layout->takeAt(0);
	ASSERT_NE(taken, nullptr);
	EXPECT_EQ(taken->widget(), a);
	EXPECT_EQ(layout->count(), 1);
	delete taken;

	EXPECT_EQ(layout->takeAt(99), nullptr);
	EXPECT_EQ(layout->takeAt(-1), nullptr);
}

TEST(WidgetLayout, FlowLayoutSpacingGetters)
{
	QWidget container;
	FlowLayout *layout = new FlowLayout(&container, 0, 7, 9);

	EXPECT_EQ(layout->horizontalSpacing(), 7);
	EXPECT_EQ(layout->verticalSpacing(), 9);
}

TEST(WidgetLayout, FlowLayoutWrapsAndComputesHeightForWidth)
{
	QWidget container;
	FlowLayout *layout = new FlowLayout(&container, 0, 0, 0);

	const int kButtonCount = 5;
	for (int i = 0; i < kButtonCount; i++) {
		auto *b = new QPushButton(QStringLiteral("Btn"));
		b->setFixedSize(100, 30);
		layout->addWidget(b);
	}

	EXPECT_TRUE(layout->hasHeightForWidth());
	EXPECT_EQ(layout->expandingDirections(), Qt::Horizontal | Qt::Vertical);

	// 1000px fits all five 100px buttons on one row; 250px fits two per row,
	// so five buttons need three rows
	EXPECT_EQ(layout->heightForWidth(1000), 30);
	EXPECT_EQ(layout->heightForWidth(250), 90);

	// Lay out for real and inspect positions
	layout->setGeometry(QRect(0, 0, 250, 90));
	ASSERT_EQ(layout->count(), kButtonCount);

	EXPECT_EQ(layout->itemAt(0)->geometry().topLeft(), QPoint(0, 0));
	EXPECT_EQ(layout->itemAt(1)->geometry().topLeft(), QPoint(100, 0));

	// Third button wraps to the second row, fifth to the third
	EXPECT_EQ(layout->itemAt(2)->geometry().topLeft(), QPoint(0, 30));
	EXPECT_EQ(layout->itemAt(4)->geometry().topLeft(), QPoint(0, 60));

	EXPECT_TRUE(layout->sizeHint().isValid());
}

TEST(WidgetLayout, ColumnedGridLayoutArrangesByMaximumColumns)
{
	QWidget container;
	olive::ColumnedGridLayout *layout =
		new olive::ColumnedGridLayout(&container, 3);

	QVector<QPushButton *> buttons;
	for (int i = 0; i < 7; i++) {
		auto *b = new QPushButton(QString::number(i));
		buttons.append(b);
		layout->Add(b);
	}

	EXPECT_EQ(layout->MaximumColumns(), 3);
	EXPECT_EQ(layout->count(), 7);

	// Widgets are placed row-major with at most three columns
	for (int i = 0; i < buttons.size(); i++) {
		QLayoutItem *item = layout->itemAtPosition(i / 3, i % 3);
		ASSERT_NE(item, nullptr) << i;
		EXPECT_EQ(item->widget(), buttons.at(i)) << i;
	}

	// Nothing beyond the last populated cell
	EXPECT_EQ(layout->itemAtPosition(2, 1), nullptr);

	layout->SetMaximumColumns(4);
	EXPECT_EQ(layout->MaximumColumns(), 4);
}

TEST(WidgetLayout, ColumnedGridLayoutWithoutColumnLimitStillAdds)
{
	QWidget container;
	olive::ColumnedGridLayout *layout = new olive::ColumnedGridLayout(&container);

	EXPECT_EQ(layout->MaximumColumns(), 0);

	auto *a = new QPushButton(QStringLiteral("A"));
	auto *b = new QPushButton(QStringLiteral("B"));
	layout->Add(a);
	layout->Add(b);

	EXPECT_EQ(layout->count(), 2);
}
