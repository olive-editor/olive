#include <gtest/gtest.h>

#include <QColor>

#include "undo/undostack.h"
#include "undo/undocommand.h"

namespace
{
class TestCommand final : public olive::UndoCommand {
public:
	explicit TestCommand(int *value)
		: value_(value)
	{
	}

	olive::Project *GetRelevantProject() const override
	{
		return nullptr;
	}

protected:
	void redo() override
	{
		if (value_) {
			(*value_)++;
		}
	}

	void undo() override
	{
		if (value_) {
			(*value_)--;
		}
	}

private:
	int *value_ = nullptr;
};
}

TEST(UndoStack, PushUndoRedo)
{
	int counter = 0;
	olive::UndoStack stack;
	stack.push(new TestCommand(&counter), QStringLiteral("Test"));
	EXPECT_EQ(counter, 1);
	stack.undo();
	EXPECT_EQ(counter, 0);
	stack.redo();
	EXPECT_EQ(counter, 1);
}

TEST(UndoStack, EmptyStateAndModelData)
{
	olive::UndoStack stack;
	EXPECT_FALSE(stack.CanUndo());
	EXPECT_FALSE(stack.CanRedo());
	EXPECT_EQ(stack.columnCount(), 2);
	EXPECT_EQ(stack.rowCount(), 1);
	EXPECT_TRUE(stack.hasChildren(QModelIndex()));

	QModelIndex name_index = stack.index(0, 1);
	EXPECT_EQ(stack.data(name_index, Qt::DisplayRole).toString(),
			  QStringLiteral("New/Open Project"));
	EXPECT_EQ(stack.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
			  QStringLiteral("Number"));
	EXPECT_EQ(stack.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
			  QStringLiteral("Action"));
}

TEST(UndoStack, UndoRedoListsAndColors)
{
	int counter = 0;
	olive::UndoStack stack;
	stack.push(new TestCommand(&counter), QStringLiteral("First"));
	stack.push(new TestCommand(&counter), QStringLiteral("Second"));
	EXPECT_EQ(counter, 2);
	EXPECT_EQ(stack.rowCount(), 3);

	stack.undo();
	EXPECT_EQ(counter, 1);
	EXPECT_TRUE(stack.CanRedo());

	QModelIndex undone_name = stack.index(2, 1);
	EXPECT_EQ(stack.data(undone_name, Qt::DisplayRole).toString(),
			  QStringLiteral("Second"));
	QColor undone_color =
		stack.data(undone_name, Qt::ForegroundRole).value<QColor>();
	EXPECT_EQ(undone_color, QColor(Qt::gray));

	stack.redo();
	EXPECT_EQ(counter, 2);
	EXPECT_FALSE(stack.CanRedo());
}

TEST(UndoStack, JumpRestoresState)
{
	int counter = 0;
	olive::UndoStack stack;
	stack.push(new TestCommand(&counter), QStringLiteral("A"));
	stack.push(new TestCommand(&counter), QStringLiteral("B"));
	stack.push(new TestCommand(&counter), QStringLiteral("C"));
	EXPECT_EQ(counter, 3);
	EXPECT_EQ(stack.rowCount(), 4);

	stack.jump(1);
	EXPECT_EQ(counter, 0);
	EXPECT_TRUE(stack.CanRedo());

	stack.jump(4);
	EXPECT_EQ(counter, 3);
	EXPECT_FALSE(stack.CanRedo());
}

TEST(UndoStack, EmptyMultiUndoCommandIsIgnored)
{
	olive::UndoStack stack;
	auto *empty_multi = new olive::MultiUndoCommand();
	stack.push(empty_multi, QStringLiteral("Empty"));
	EXPECT_EQ(stack.rowCount(), 1);
	EXPECT_FALSE(stack.CanUndo());
}

TEST(UndoStack, MultipleUndosAndRedos)
{
	int counter = 0;
	olive::UndoStack stack;
	stack.push(new TestCommand(&counter), QStringLiteral("One"));
	stack.push(new TestCommand(&counter), QStringLiteral("Two"));
	stack.push(new TestCommand(&counter), QStringLiteral("Three"));
	EXPECT_EQ(counter, 3);

	stack.undo();
	stack.undo();
	EXPECT_EQ(counter, 1);
	EXPECT_TRUE(stack.CanUndo());
	EXPECT_TRUE(stack.CanRedo());

	stack.redo();
	EXPECT_EQ(counter, 2);
}

TEST(UndoStack, PushAfterUndoClearsRedoBranch)
{
	int counter = 0;
	olive::UndoStack stack;
	stack.push(new TestCommand(&counter), QStringLiteral("First"));
	stack.push(new TestCommand(&counter), QStringLiteral("Second"));
	EXPECT_EQ(counter, 2);

	stack.undo();
	EXPECT_EQ(counter, 1);

	stack.push(new TestCommand(&counter), QStringLiteral("Third"));
	EXPECT_EQ(counter, 2);
	EXPECT_FALSE(stack.CanRedo());
}

TEST(UndoStack, ResetClearsHistory)
{
	int counter = 0;
	olive::UndoStack stack;
	stack.push(new TestCommand(&counter), QStringLiteral("Action"));
	EXPECT_EQ(counter, 1);

	stack.clear();
	EXPECT_FALSE(stack.CanUndo());
	EXPECT_FALSE(stack.CanRedo());
	EXPECT_EQ(stack.rowCount(), 1);
}

TEST(UndoStack, MultiUndoCommandWithChildren)
{
	int counter = 0;
	olive::UndoStack stack;

	auto *multi = new olive::MultiUndoCommand();
	multi->add_child(new TestCommand(&counter));
	multi->add_child(new TestCommand(&counter));
	stack.push(multi, QStringLiteral("Multi"));

	EXPECT_EQ(counter, 2);
	stack.undo();
	EXPECT_EQ(counter, 0);
	stack.redo();
	EXPECT_EQ(counter, 2);
}
