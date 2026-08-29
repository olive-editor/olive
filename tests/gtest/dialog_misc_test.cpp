#include <gtest/gtest.h>

#include <memory>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QListWidget>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTreeWidget>

#include "common/filefunctions.h"
#include "config/config.h"
#include "core.h"
#include "dialog/about/about.h"
#include "dialog/actionsearch/actionsearch.h"
#include "dialog/autorecovery/autorecoverydialog.h"
#include "dialog/color/colordialog.h"
#include "dialog/configbase/configdialogbase.h"
#include "dialog/diskcache/diskcachedialog.h"
#include "dialog/preferences/keysequenceeditor.h"
#include "dialog/preferences/tabs/preferencesappearancetab.h"
#include "dialog/preferences/tabs/preferencesdisktab.h"
#include "dialog/preferences/tabs/preferencesluttab.h"
#include "dialog/progress/progress.h"
#include "dialog/rendercancel/rendercancel.h"
#include "dialog/task/task.h"
#include "dialog/text/text.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "render/diskmanager.h"

namespace
{

void EnsureAppSingletons()
{
	if (!olive::Core::instance()) {
		new olive::Core(olive::Core::CoreParams()); // intentionally leaked
	}
	if (!olive::DiskManager::instance()) {
		olive::DiskManager::CreateInstance();
	}
}

// Redirects QStandardPaths (config/autorecovery locations) to a disposable
// test location for the lifetime of the guard
class StandardPathsTestModeGuard {
public:
	StandardPathsTestModeGuard()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	~StandardPathsTestModeGuard()
	{
		QStandardPaths::setTestModeEnabled(false);
	}
};

class DummyTab : public olive::ConfigDialogBaseTab {
public:
	bool validate_result = true;
	int accept_count = 0;

	virtual bool Validate() override
	{
		return validate_result;
	}

	virtual void Accept(olive::MultiUndoCommand *) override
	{
		++accept_count;
	}
};

class TestConfigDialog : public olive::ConfigDialogBase {
public:
	using olive::ConfigDialogBase::AddTab;
	using olive::ConfigDialogBase::ConfigDialogBase;

	bool accept_event_called = false;

protected:
	virtual void AcceptEvent() override
	{
		accept_event_called = true;
	}
};

class DummyTask : public olive::Task {
public:
	DummyTask()
	{
		SetTitle(QStringLiteral("DummyTask"));
	}

protected:
	virtual bool Run() override
	{
		return true;
	}
};

} // namespace

//
// about
//
TEST(DialogAbout, WelcomeDialogHasDontShowAgainCheckbox)
{
	olive::AboutDialog welcome(true);
	EXPECT_NE(welcome.findChild<QCheckBox *>(), nullptr);

	olive::AboutDialog about(false);
	EXPECT_EQ(about.findChild<QCheckBox *>(), nullptr);
}

TEST(DialogAbout, AcceptWithCheckboxWritesConfig)
{
	const QVariant old_value =
		olive::Config::Current()[QStringLiteral("ShowWelcomeDialog")];

	{
		olive::AboutDialog welcome(true);
		QCheckBox *box = welcome.findChild<QCheckBox *>();
		ASSERT_NE(box, nullptr);
		box->setChecked(true);
		welcome.accept();
	}

	EXPECT_FALSE(
		olive::Config::Current()[QStringLiteral("ShowWelcomeDialog")].toBool());

	olive::Config::Current()[QStringLiteral("ShowWelcomeDialog")] = old_value;
}

//
// actionsearch
//
TEST(DialogActionSearch, SearchFiltersActionsCaseInsensitively)
{
	QWidget parent;
	olive::ActionSearch dialog(&parent);

	QMenuBar bar;
	QMenu *file_menu = bar.addMenu(QStringLiteral("&File"));
	file_menu->addAction(QStringLiteral("New Project"));
	file_menu->addAction(QStringLiteral("&Open..."));
	file_menu->addSeparator();
	QMenu *recent_menu = file_menu->addMenu(QStringLiteral("Recent"));
	recent_menu->addAction(QStringLiteral("Project A"));
	bar.addMenu(QStringLiteral("&Edit"))->addAction(QStringLiteral("Undo"));

	dialog.SetMenuBar(&bar);

	auto *entry = dialog.findChild<olive::ActionSearchEntry *>();
	auto *list = dialog.findChild<olive::ActionSearchList *>();
	ASSERT_NE(entry, nullptr);
	ASSERT_NE(list, nullptr);

	entry->setText(QStringLiteral("OPEN"));
	ASSERT_EQ(list->count(), 1);
	// Item text is "action\n(menu hierarchy)" with accelerator '&' stripped
	EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("Open...")));
	EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("(File)")));
	// First match is auto-selected for keyboard use
	EXPECT_TRUE(list->item(0)->isSelected());

	entry->setText(QStringLiteral("zzzz-no-match"));
	EXPECT_EQ(list->count(), 0);

	// Nested menus are searched recursively
	entry->setText(QStringLiteral("project a"));
	ASSERT_EQ(list->count(), 1);
	EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("Recent")));
}

TEST(DialogActionSearch, PerformActionTriggersSelectedAction)
{
	QWidget parent;
	olive::ActionSearch dialog(&parent);

	QMenuBar bar;
	QMenu *file_menu = bar.addMenu(QStringLiteral("&File"));
	QAction *open_action = file_menu->addAction(QStringLiteral("Open..."));

	dialog.SetMenuBar(&bar);

	bool triggered = false;
	QObject::connect(open_action, &QAction::triggered,
					 [&triggered]() { triggered = true; });

	auto *entry = dialog.findChild<olive::ActionSearchEntry *>();
	entry->setText(QStringLiteral("open"));

	QMetaObject::invokeMethod(&dialog, "perform_action");

	EXPECT_TRUE(triggered);
	EXPECT_EQ(dialog.result(), QDialog::Accepted);
}

TEST(DialogActionSearch, SelectionMovesUpAndDown)
{
	QWidget parent;
	olive::ActionSearch dialog(&parent);

	QMenuBar bar;
	QMenu *file_menu = bar.addMenu(QStringLiteral("&File"));
	file_menu->addAction(QStringLiteral("Alpha"));
	file_menu->addAction(QStringLiteral("Beta"));

	dialog.SetMenuBar(&bar);

	auto *entry = dialog.findChild<olive::ActionSearchEntry *>();
	auto *list = dialog.findChild<olive::ActionSearchList *>();
	entry->setText(QStringLiteral("a"));
	ASSERT_GE(list->count(), 2);
	ASSERT_TRUE(list->item(0)->isSelected());

	QMetaObject::invokeMethod(&dialog, "move_selection_down");
	EXPECT_TRUE(list->item(1)->isSelected());

	QMetaObject::invokeMethod(&dialog, "move_selection_up");
	EXPECT_TRUE(list->item(0)->isSelected());

	// Already at the top: moving up again is a no-op
	QMetaObject::invokeMethod(&dialog, "move_selection_up");
	EXPECT_TRUE(list->item(0)->isSelected());
}

//
// autorecovery
//
TEST(DialogAutoRecovery, PopulatesTreeFromRecoveryFolders)
{
	StandardPathsTestModeGuard test_mode;
	EnsureAppSingletons();

	const QString root = olive::FileFunctions::GetAutoRecoveryRoot();
	const QString folder = QStringLiteral("uuid-abc");
	QDir recovery_dir(QDir(root).filePath(folder));
	ASSERT_TRUE(recovery_dir.mkpath(QStringLiteral(".")));

	QFile realname(recovery_dir.filePath(QStringLiteral("realname.txt")));
	ASSERT_TRUE(realname.open(QFile::WriteOnly));
	realname.write("My Project");
	realname.close();

	QFile newest(recovery_dir.filePath(QStringLiteral("1700000000.ove")));
	ASSERT_TRUE(newest.open(QFile::WriteOnly));
	newest.write("x");
	newest.close();

	QFile oldest(recovery_dir.filePath(QStringLiteral("1699999000.ove")));
	ASSERT_TRUE(oldest.open(QFile::WriteOnly));
	oldest.write("x");
	oldest.close();

	// Non-project files must be ignored
	QFile notes(recovery_dir.filePath(QStringLiteral("notes.txt")));
	ASSERT_TRUE(notes.open(QFile::WriteOnly));
	notes.write("x");
	notes.close();

	{
		olive::AutoRecoveryDialog dialog(QStringLiteral("message"), { folder },
										 true, nullptr);

		auto *tree = dialog.findChild<QTreeWidget *>();
		ASSERT_NE(tree, nullptr);
		ASSERT_EQ(tree->topLevelItemCount(), 1);

		QTreeWidgetItem *top = tree->topLevelItem(0);
		// Pretty name comes from realname.txt
		EXPECT_EQ(top->text(0), QStringLiteral("My Project"));
		ASSERT_EQ(top->childCount(), 2);

		// Entries are newest-first; autocheck_latest checks only the newest
		EXPECT_EQ(top->child(0)->checkState(0), Qt::Checked);
		EXPECT_EQ(top->child(1)->checkState(0), Qt::Unchecked);

		// Timestamped filenames are shown as a formatted date/time
		EXPECT_EQ(top->child(0)->text(0),
				  QDateTime::fromSecsSinceEpoch(1700000000).toString());
		EXPECT_TRUE(top->child(0)
						->data(0, Qt::UserRole)
						.toString()
						.endsWith(QStringLiteral("1700000000.ove")));
	}

	{
		// Without autocheck_latest nothing is pre-checked
		olive::AutoRecoveryDialog dialog(QStringLiteral("message"), { folder },
										 false, nullptr);

		auto *tree = dialog.findChild<QTreeWidget *>();
		QTreeWidgetItem *top = tree->topLevelItem(0);
		EXPECT_EQ(top->child(0)->checkState(0), Qt::Unchecked);
		EXPECT_EQ(top->child(1)->checkState(0), Qt::Unchecked);

		// Accepting with nothing checked must not attempt to open anything
		dialog.accept();
		EXPECT_EQ(dialog.result(), QDialog::Accepted);
	}

	QDir(root).removeRecursively();
}

TEST(DialogAutoRecovery, MissingRealnameFallsBackToFolderName)
{
	StandardPathsTestModeGuard test_mode;
	EnsureAppSingletons();

	const QString root = olive::FileFunctions::GetAutoRecoveryRoot();
	const QString folder = QStringLiteral("uuid-no-realname");
	QDir recovery_dir(QDir(root).filePath(folder));
	ASSERT_TRUE(recovery_dir.mkpath(QStringLiteral(".")));

	QFile recovery(recovery_dir.filePath(QStringLiteral("1700000000.ove")));
	ASSERT_TRUE(recovery.open(QFile::WriteOnly));
	recovery.write("x");
	recovery.close();

	olive::AutoRecoveryDialog dialog(QStringLiteral("message"), { folder },
									 false, nullptr);

	auto *tree = dialog.findChild<QTreeWidget *>();
	ASSERT_NE(tree, nullptr);
	ASSERT_EQ(tree->topLevelItemCount(), 1);
	EXPECT_EQ(tree->topLevelItem(0)->text(0), folder);

	QDir(root).removeRecursively();
}

//
// configbase
//
TEST(DialogConfigBase, AddTabPopulatesListAndStack)
{
	TestConfigDialog dialog;

	auto *tab_a = new DummyTab();
	auto *tab_b = new DummyTab();
	dialog.AddTab(tab_a, QStringLiteral("First"));
	dialog.AddTab(tab_b, QStringLiteral("Second"));

	auto *list = dialog.findChild<QListWidget *>();
	auto *stack = dialog.findChild<QStackedWidget *>();
	ASSERT_NE(list, nullptr);
	ASSERT_NE(stack, nullptr);

	EXPECT_EQ(list->count(), 2);
	EXPECT_EQ(stack->count(), 2);

	dialog.SetCurrentTab(1);
	EXPECT_EQ(list->currentRow(), 1);
	EXPECT_EQ(stack->currentIndex(), 1);
}

TEST(DialogConfigBase, AcceptCallsTabsAndAcceptEvent)
{
	EnsureAppSingletons();

	TestConfigDialog dialog;
	auto *tab_a = new DummyTab();
	auto *tab_b = new DummyTab();
	dialog.AddTab(tab_a, QStringLiteral("First"));
	dialog.AddTab(tab_b, QStringLiteral("Second"));

	// accept() is a private slot, invoke it through the meta-object
	QMetaObject::invokeMethod(&dialog, "accept");

	EXPECT_EQ(tab_a->accept_count, 1);
	EXPECT_EQ(tab_b->accept_count, 1);
	EXPECT_TRUE(dialog.accept_event_called);
	EXPECT_EQ(dialog.result(), QDialog::Accepted);
}

TEST(DialogConfigBase, FailedValidateBlocksAccept)
{
	EnsureAppSingletons();

	TestConfigDialog dialog;
	auto *tab_a = new DummyTab();
	auto *tab_b = new DummyTab();
	tab_a->validate_result = false;
	dialog.AddTab(tab_a, QStringLiteral("First"));
	dialog.AddTab(tab_b, QStringLiteral("Second"));

	// accept() is a private slot, invoke it through the meta-object
	QMetaObject::invokeMethod(&dialog, "accept");

	EXPECT_EQ(tab_a->accept_count, 0);
	EXPECT_EQ(tab_b->accept_count, 0);
	EXPECT_FALSE(dialog.accept_event_called);
	EXPECT_NE(dialog.result(), QDialog::Accepted);
}

//
// diskcache
//
TEST(DialogDiskCache, AcceptAppliesLimitAndClearOnClose)
{
	QTemporaryDir temp_dir;
	ASSERT_TRUE(temp_dir.isValid());

	olive::DiskCacheFolder folder(temp_dir.path());

	olive::DiskCacheDialog dialog(&folder);

	auto *limit_slider = dialog.findChild<olive::FloatSlider *>();
	auto *clear_box = dialog.findChild<QCheckBox *>();
	ASSERT_NE(limit_slider, nullptr);
	ASSERT_NE(clear_box, nullptr);

	// Fields reflect the folder's current settings (20 GB default)
	EXPECT_DOUBLE_EQ(limit_slider->GetValue(), 20.0);
	EXPECT_FALSE(clear_box->isChecked());

	limit_slider->SetValue(5.0);
	clear_box->setChecked(true);

	dialog.accept();

	EXPECT_EQ(folder.GetLimit(),
			  5 * static_cast<qint64>(olive::kBytesInGigabyte));
	EXPECT_TRUE(folder.GetClearOnClose());
}

//
// text
//
TEST(DialogText, TextIsReadBackFromEditor)
{
	olive::TextDialog dialog(QStringLiteral("Hello"));
	EXPECT_EQ(dialog.text(), QStringLiteral("Hello"));

	auto *edit = dialog.findChild<QPlainTextEdit *>();
	ASSERT_NE(edit, nullptr);
	edit->setPlainText(QStringLiteral("World"));
	EXPECT_EQ(dialog.text(), QStringLiteral("World"));
}

//
// progress
//
TEST(DialogProgress, CancelButtonEmitsCancelledAndDisables)
{
	olive::ProgressDialog dialog(QStringLiteral("Working..."),
								 QStringLiteral("Title"));
	EXPECT_EQ(dialog.windowTitle(), QStringLiteral("Title"));

	QPushButton *cancel_btn = nullptr;
	foreach (QPushButton *btn, dialog.findChildren<QPushButton *>()) {
		if (btn->text() == QStringLiteral("Cancel")) {
			cancel_btn = btn;
			break;
		}
	}
	ASSERT_NE(cancel_btn, nullptr);

	bool cancelled = false;
	QObject::connect(&dialog, &olive::ProgressDialog::Cancelled,
					 [&cancelled]() { cancelled = true; });

	cancel_btn->click();

	EXPECT_TRUE(cancelled);
	EXPECT_FALSE(cancel_btn->isEnabled());
}

//
// rendercancel
//
TEST(DialogRenderCancel, IdleWorkersDoNotBlock)
{
	olive::RenderCancelDialog dialog;

	dialog.SetWorkerCount(2);
	dialog.WorkerStarted();
	dialog.WorkerDone();

	// No busy workers: must return immediately without exec()ing
	dialog.RunIfWorkersAreBusy();

	EXPECT_FALSE(dialog.isVisible());
}

//
// task
//
TEST(DialogTask, WrapsAndOwnsTask)
{
	auto *task = new DummyTask();
	QPointer<olive::Task> task_guard(task);

	auto *dialog = new olive::TaskDialog(task, QStringLiteral("Title"));

	EXPECT_EQ(dialog->GetTask(), task);
	EXPECT_EQ(task->parent(), dialog);

	// The dialog takes ownership of the task
	delete dialog;
	EXPECT_TRUE(task_guard.isNull());
}

//
// color
//
TEST(DialogColor, SelectedColorRoundTrips)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ColorDialog dialog(project.color_manager(),
							  olive::Color(1.0f, 0.0f, 0.0f, 1.0f));

	olive::ManagedColor selected = dialog.GetSelectedColor();
	EXPECT_GT(selected.red(), selected.green());
	EXPECT_GT(selected.red(), selected.blue());

	dialog.SetColor(olive::Color(0.0f, 0.0f, 1.0f, 1.0f));

	olive::ManagedColor blue = dialog.GetSelectedColor();
	EXPECT_GT(blue.blue(), blue.red());
	EXPECT_GT(blue.blue(), blue.green());
}

//
// preferences (remaining tabs)
//
TEST(PreferencesAppearanceTab, ContainsStyleAndColorChoices)
{
	olive::PreferencesAppearanceTab tab;

	EXPECT_FALSE(tab.findChildren<QComboBox *>().isEmpty());
	EXPECT_FALSE(tab.findChildren<olive::ColorCodingComboBox *>().isEmpty());
}

TEST(PreferencesDiskTab, ValidatesUnchangedCacheLocation)
{
	EnsureAppSingletons();

	olive::PreferencesDiskTab tab;

	// Unchanged location must validate without prompting
	EXPECT_TRUE(tab.Validate());
}

TEST(PreferencesLutTab, ConstructsWithDirectoryList)
{
	olive::PreferencesLutTab tab;

	EXPECT_NE(tab.findChild<QListWidget *>(), nullptr);
}

TEST(DialogKeySequenceEditor, TransfersShortcutsToAndFromAction)
{
	QAction action(QStringLiteral("Test Action"));
	action.setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
	action.setProperty("id", QStringLiteral("test.action"));
	action.setProperty("keydefault", QKeySequence(QStringLiteral("Ctrl+T")));

	olive::KeySequenceEditor editor(nullptr, &action);

	// Editor initializes from the action's current shortcut
	EXPECT_EQ(editor.keySequence(), QKeySequence(QStringLiteral("Ctrl+T")));
	EXPECT_EQ(editor.action_name(), QStringLiteral("test.action"));

	// Shortcut matching the default does not need to be saved
	EXPECT_TRUE(editor.export_shortcut().isEmpty());

	editor.setKeySequence(QKeySequence(QStringLiteral("Ctrl+U")));
	EXPECT_EQ(editor.export_shortcut(),
			  QStringLiteral("test.action\tCtrl+U"));

	// The action is only updated when set_action_shortcut() is called
	EXPECT_EQ(action.shortcut(), QKeySequence(QStringLiteral("Ctrl+T")));
	editor.set_action_shortcut();
	EXPECT_EQ(action.shortcut(), QKeySequence(QStringLiteral("Ctrl+U")));

	editor.reset_to_default();
	EXPECT_EQ(editor.keySequence(), QKeySequence(QStringLiteral("Ctrl+T")));
}
