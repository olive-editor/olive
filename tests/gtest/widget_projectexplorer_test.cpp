#include <gtest/gtest.h>

#include <memory>

#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QSignalSpy>

#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/output/track/track.h"
#include "node/project.h"
#include "node/project/folder/folder.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "render/diskmanager.h"
#include "widget/projectexplorer/projectexplorer.h"
#include "widget/projectexplorer/projectviewmodel.h"
#include "widget/projecttoolbar/projecttoolbar.h"

using namespace olive;

namespace
{

// Renames and moves go through the global undo stack hosted by Core
void EnsureAppSingletons()
{
	if (!olive::Core::instance()) {
		new olive::Core(olive::Core::CoreParams()); // intentionally leaked
	}
	if (!olive::DiskManager::instance()) {
		olive::DiskManager::CreateInstance();
	}
}

} // namespace

class ProjectViewModelTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();
		EnsureAppSingletons();

		project_ = std::make_unique<Project>();
		project_->Initialize();

		model_.set_project(project_.get());
	}

	template <typename T> T *AddItem(Folder *parent)
	{
		auto *node = new T();
		node->setParent(project_.get());
		FolderAddChild(parent, node).redo_now();
		return node;
	}

	std::unique_ptr<Project> project_;
	ProjectViewModel model_{ nullptr };
};

TEST_F(ProjectViewModelTest, ModelWithoutProjectIsEmpty)
{
	ProjectViewModel empty(nullptr);
	EXPECT_EQ(empty.rowCount(), 0);
	EXPECT_EQ(empty.columnCount(), 0);
	EXPECT_EQ(empty.project(), nullptr);
}

TEST_F(ProjectViewModelTest, HierarchyIndexesAndParents)
{
	Folder *folder = AddItem<Folder>(project_->root());
	Footage *footage = AddItem<Footage>(project_->root());
	Sequence *sequence = AddItem<Sequence>(project_->root());
	Footage *nested = AddItem<Footage>(folder);

	ASSERT_EQ(model_.rowCount(), 3);
	EXPECT_EQ(model_.columnCount(), ProjectViewModel::kColumnCount);

	// Root children appear in insertion order
	EXPECT_EQ(model_.index(0, 0).internalPointer(), folder);
	EXPECT_EQ(model_.index(1, 0).internalPointer(), footage);
	EXPECT_EQ(model_.index(2, 0).internalPointer(), sequence);

	// Children of the root report an invalid parent index
	EXPECT_EQ(model_.parent(model_.index(0, 0)), QModelIndex());

	// Nested items resolve to their folder
	EXPECT_EQ(model_.rowCount(model_.index(0, 0)), 1);
	QModelIndex nested_index = model_.index(0, 0, model_.index(0, 0));
	EXPECT_EQ(nested_index.internalPointer(), nested);
	EXPECT_EQ(model_.parent(nested_index), model_.index(0, 0));

	// CreateIndexFromItem round-trips through the same object
	EXPECT_EQ(model_.CreateIndexFromItem(footage), model_.index(1, 0));
	EXPECT_EQ(model_.CreateIndexFromItem(nested).internalPointer(), nested);

	// Only folders report children, even when empty
	EXPECT_TRUE(model_.hasChildren(model_.index(0, 0)));
	EXPECT_FALSE(model_.hasChildren(model_.index(1, 0)));
}

TEST_F(ProjectViewModelTest, ItemInsertAndRemoveEmitModelSignals)
{
	QSignalSpy about_to_insert(&model_, &QAbstractItemModel::rowsAboutToBeInserted);
	QSignalSpy inserted(&model_, &QAbstractItemModel::rowsInserted);
	QSignalSpy about_to_remove(&model_, &QAbstractItemModel::rowsAboutToBeRemoved);
	QSignalSpy removed(&model_, &QAbstractItemModel::rowsRemoved);

	Footage *footage = AddItem<Footage>(project_->root());
	EXPECT_EQ(about_to_insert.count(), 1);
	EXPECT_EQ(inserted.count(), 1);
	EXPECT_EQ(model_.rowCount(), 1);

	// Deleting the node disconnects the folder edge and removes the row
	delete footage;
	EXPECT_EQ(about_to_remove.count(), 1);
	EXPECT_EQ(removed.count(), 1);
	EXPECT_EQ(model_.rowCount(), 0);
}

TEST_F(ProjectViewModelTest, DataColumnsAndHeader)
{
	Folder *folder = AddItem<Folder>(project_->root());
	folder->SetLabel(QStringLiteral("Media"));

	QModelIndex name_index = model_.CreateIndexFromItem(folder, ProjectViewModel::kName);
	EXPECT_EQ(model_.data(name_index, Qt::DisplayRole).toString(),
			  QStringLiteral("Media"));
	EXPECT_EQ(model_.data(name_index, Qt::EditRole).toString(),
			  QStringLiteral("Media"));
	EXPECT_EQ(model_.data(name_index, ProjectViewModel::kInnerTextRole).toString(),
			  QStringLiteral("Media"));

	// A folder carries no duration/rate/timestamps
	EXPECT_FALSE(model_.data(model_.CreateIndexFromItem(folder, ProjectViewModel::kDuration),
							 Qt::DisplayRole)
					 .isValid());
	EXPECT_FALSE(model_.data(model_.CreateIndexFromItem(folder, ProjectViewModel::kRate),
							 Qt::DisplayRole)
					 .isValid());

	// EditRole is only served for the name column
	EXPECT_FALSE(model_.data(model_.CreateIndexFromItem(folder, ProjectViewModel::kDuration),
							 Qt::EditRole)
					 .isValid());

	EXPECT_EQ(model_.headerData(ProjectViewModel::kName, Qt::Horizontal).toString(),
			  QStringLiteral("Name"));
	EXPECT_EQ(model_.headerData(ProjectViewModel::kDuration, Qt::Horizontal).toString(),
			  QStringLiteral("Duration"));
	EXPECT_EQ(model_.headerData(ProjectViewModel::kRate, Qt::Horizontal).toString(),
			  QStringLiteral("Rate"));
	EXPECT_EQ(model_.headerData(ProjectViewModel::kLastModified, Qt::Horizontal).toString(),
			  QStringLiteral("Modified"));
	EXPECT_EQ(model_.headerData(ProjectViewModel::kCreatedTime, Qt::Horizontal).toString(),
			  QStringLiteral("Created"));
}

TEST_F(ProjectViewModelTest, FlagsMarkNameEditableAndFoldersDroppable)
{
	Folder *folder = AddItem<Folder>(project_->root());
	Footage *footage = AddItem<Footage>(project_->root());

	const Qt::ItemFlags folder_name_flags =
		model_.flags(model_.CreateIndexFromItem(folder, ProjectViewModel::kName));
	EXPECT_TRUE(folder_name_flags & Qt::ItemIsEditable);
	EXPECT_TRUE(folder_name_flags & Qt::ItemIsDragEnabled);
	EXPECT_TRUE(folder_name_flags & Qt::ItemIsDropEnabled);

	// Non-name columns are not editable, non-folders do not accept drops
	const Qt::ItemFlags footage_duration_flags =
		model_.flags(model_.CreateIndexFromItem(footage, ProjectViewModel::kDuration));
	EXPECT_FALSE(footage_duration_flags & Qt::ItemIsEditable);
	EXPECT_FALSE(footage_duration_flags & Qt::ItemIsDropEnabled);

	// The background accepts external file drops
	EXPECT_EQ(model_.flags(QModelIndex()), Qt::ItemIsDropEnabled);
}

TEST_F(ProjectViewModelTest, SetDataRenamesItemThroughUndoStack)
{
	Folder *folder = AddItem<Folder>(project_->root());
	folder->SetLabel(QStringLiteral("Before"));

	QSignalSpy data_changed(&model_, &QAbstractItemModel::dataChanged);

	QModelIndex name_index = model_.CreateIndexFromItem(folder, ProjectViewModel::kName);
	EXPECT_TRUE(model_.setData(name_index, QStringLiteral("After"), Qt::EditRole));
	EXPECT_EQ(folder->GetLabel(), QStringLiteral("After"));
	EXPECT_GE(data_changed.count(), 1);

	// The rename is a regular undo command
	Core::instance()->undo_stack()->undo();
	EXPECT_EQ(folder->GetLabel(), QStringLiteral("Before"));
	Core::instance()->undo_stack()->clear();

	// Empty names and other columns are rejected
	EXPECT_FALSE(model_.setData(name_index, QString(), Qt::EditRole));
	EXPECT_FALSE(model_.setData(model_.CreateIndexFromItem(folder, ProjectViewModel::kRate),
								QStringLiteral("After"), Qt::EditRole));
	EXPECT_EQ(folder->GetLabel(), QStringLiteral("Before"));
}

TEST_F(ProjectViewModelTest, MimeDataEncodesEachRowOnce)
{
	EXPECT_EQ(model_.mimeTypes(),
			  (QStringList{ Project::kItemMimeType, QStringLiteral("text/uri-list") }));

	Footage *footage = AddItem<Footage>(project_->root());
	Folder *folder = AddItem<Folder>(project_->root());

	// Passing every column of two rows must still encode only two items
	QModelIndexList indexes{ model_.CreateIndexFromItem(footage, ProjectViewModel::kName),
							 model_.CreateIndexFromItem(footage, ProjectViewModel::kDuration),
							 model_.CreateIndexFromItem(folder, ProjectViewModel::kName) };
	std::unique_ptr<QMimeData> mime(model_.mimeData(indexes));
	ASSERT_NE(mime, nullptr);
	ASSERT_TRUE(mime->hasFormat(Project::kItemMimeType));

	QByteArray encoded = mime->data(Project::kItemMimeType);
	QDataStream stream(&encoded, QIODevice::ReadOnly);
	QVector<Track::Reference> streams;
	quintptr ptr = 0;

	stream >> streams >> ptr;
	EXPECT_EQ(reinterpret_cast<Node *>(ptr), footage);

	stream >> streams >> ptr;
	EXPECT_EQ(reinterpret_cast<Node *>(ptr), folder);

	EXPECT_TRUE(stream.atEnd());

	// An empty selection produces no mime data
	EXPECT_EQ(model_.mimeData(QModelIndexList()), nullptr);
}

TEST_F(ProjectViewModelTest, DropMimeDataMovesItemIntoFolder)
{
	Folder *folder = AddItem<Folder>(project_->root());
	Footage *footage = AddItem<Footage>(project_->root());
	ASSERT_EQ(model_.rowCount(), 2);

	std::unique_ptr<QMimeData> mime(
		model_.mimeData({ model_.CreateIndexFromItem(footage) }));
	ASSERT_NE(mime, nullptr);

	EXPECT_TRUE(model_.dropMimeData(mime.get(), Qt::CopyAction, -1, -1,
									model_.CreateIndexFromItem(folder)));
	EXPECT_EQ(footage->folder(), folder);
	EXPECT_EQ(model_.rowCount(), 1);
	EXPECT_EQ(model_.rowCount(model_.CreateIndexFromItem(folder)), 1);

	// The move is undoable
	Core::instance()->undo_stack()->undo();
	EXPECT_EQ(footage->folder(), project_->root());
	EXPECT_EQ(model_.rowCount(), 2);
	Core::instance()->undo_stack()->clear();
}

TEST_F(ProjectViewModelTest, DropRejectsNonFolderAndSelfNesting)
{
	Folder *folder = AddItem<Folder>(project_->root());
	Folder *subfolder = AddItem<Folder>(folder);
	Footage *footage = AddItem<Footage>(project_->root());

	// Cannot drop onto a non-folder item
	std::unique_ptr<QMimeData> footage_mime(
		model_.mimeData({ model_.CreateIndexFromItem(footage) }));
	EXPECT_FALSE(model_.dropMimeData(footage_mime.get(), Qt::CopyAction, -1, -1,
									 model_.CreateIndexFromItem(footage)));
	EXPECT_EQ(footage->folder(), project_->root());

	// Dropping a folder into its own descendant is skipped as a no-op
	std::unique_ptr<QMimeData> folder_mime(
		model_.mimeData({ model_.CreateIndexFromItem(folder) }));
	EXPECT_TRUE(model_.dropMimeData(folder_mime.get(), Qt::CopyAction, -1, -1,
									model_.CreateIndexFromItem(subfolder)));
	EXPECT_EQ(folder->folder(), project_->root());

	// Dropping onto the background moves items to the root
	std::unique_ptr<QMimeData> sub_mime(
		model_.mimeData({ model_.CreateIndexFromItem(subfolder) }));
	EXPECT_TRUE(model_.dropMimeData(sub_mime.get(), Qt::CopyAction, -1, -1,
									QModelIndex()));
	EXPECT_EQ(subfolder->folder(), project_->root());
	Core::instance()->undo_stack()->clear();
}

class ProjectExplorerTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();
		EnsureAppSingletons();

		project_ = std::make_unique<Project>();
		project_->Initialize();
	}

	template <typename T> T *AddItem(Folder *parent)
	{
		auto *node = new T();
		node->setParent(project_.get());
		FolderAddChild(parent, node).redo_now();
		return node;
	}

	std::unique_ptr<Project> project_;
};

TEST_F(ProjectExplorerTest, SetProjectAndSwitchViewType)
{
	ProjectExplorer explorer(nullptr);
	EXPECT_EQ(explorer.project(), nullptr);

	explorer.set_project(project_.get());
	EXPECT_EQ(explorer.project(), project_.get());

	// Tree view is the default
	EXPECT_EQ(explorer.view_type(), ProjectToolbar::TreeView);

	explorer.set_view_type(ProjectToolbar::ListView);
	EXPECT_EQ(explorer.view_type(), ProjectToolbar::ListView);

	explorer.set_view_type(ProjectToolbar::IconView);
	EXPECT_EQ(explorer.view_type(), ProjectToolbar::IconView);
}

TEST_F(ProjectExplorerTest, GetSelectedFolderFallsBackToRoot)
{
	Folder *folder = AddItem<Folder>(project_->root());
	Footage *footage = AddItem<Footage>(project_->root());

	ProjectExplorer explorer(nullptr);
	explorer.set_project(project_.get());

	// No selection: heuristic returns the project root
	EXPECT_EQ(explorer.GetSelectedFolder(), project_->root());

	// A selected folder is returned directly
	EXPECT_TRUE(explorer.SelectItem(folder));
	EXPECT_EQ(explorer.GetSelectedFolder(), folder);

	// A selected non-folder resolves to its parent folder
	EXPECT_TRUE(explorer.SelectItem(footage));
	EXPECT_EQ(explorer.GetSelectedFolder(), project_->root());
}

TEST_F(ProjectExplorerTest, SelectItemUpdatesSelectedItems)
{
	Footage *footage = AddItem<Footage>(project_->root());

	ProjectExplorer explorer(nullptr);
	explorer.set_project(project_.get());

	EXPECT_TRUE(explorer.SelectedItems().isEmpty());

	EXPECT_TRUE(explorer.SelectItem(footage));
	EXPECT_EQ(explorer.SelectedItems().size(), 1);
	EXPECT_EQ(explorer.SelectedItems().first(), footage);

	explorer.DeselectAll();
	EXPECT_TRUE(explorer.SelectedItems().isEmpty());
}

class ProjectToolbarTest : public ::testing::Test {
};

TEST_F(ProjectToolbarTest, ActionButtonsEmitSignals)
{
	ProjectToolbar toolbar(nullptr);

	// Buttons in creation order: new, open, save, tree, list, icon
	const QList<QPushButton *> buttons = toolbar.findChildren<QPushButton *>();
	ASSERT_EQ(buttons.size(), 6);

	QSignalSpy new_spy(&toolbar, &ProjectToolbar::NewClicked);
	QSignalSpy open_spy(&toolbar, &ProjectToolbar::OpenClicked);
	QSignalSpy save_spy(&toolbar, &ProjectToolbar::SaveClicked);

	buttons.at(0)->click();
	EXPECT_EQ(new_spy.count(), 1);

	buttons.at(1)->click();
	EXPECT_EQ(open_spy.count(), 1);

	buttons.at(2)->click();
	EXPECT_EQ(save_spy.count(), 1);
}

TEST_F(ProjectToolbarTest, SearchFieldForwardsTextChanges)
{
	ProjectToolbar toolbar(nullptr);

	auto *search = toolbar.findChild<QLineEdit *>();
	ASSERT_NE(search, nullptr);

	QSignalSpy search_spy(&toolbar, &ProjectToolbar::SearchChanged);

	search->setText(QStringLiteral("media"));
	ASSERT_EQ(search_spy.count(), 1);
	EXPECT_EQ(search_spy.first().first().toString(), QStringLiteral("media"));
}

TEST_F(ProjectToolbarTest, ViewButtonsAreExclusiveAndEmitViewChanged)
{
	ProjectToolbar toolbar(nullptr);
	const QList<QPushButton *> buttons = toolbar.findChildren<QPushButton *>();
	ASSERT_EQ(buttons.size(), 6);

	QPushButton *tree_button = buttons.at(3);
	QPushButton *list_button = buttons.at(4);
	QPushButton *icon_button = buttons.at(5);

	ProjectToolbar::ViewType received = ProjectToolbar::TreeView;
	int emissions = 0;
	QObject::connect(&toolbar, &ProjectToolbar::ViewChanged,
					 [&received, &emissions](ProjectToolbar::ViewType type) {
						 received = type;
						 ++emissions;
					 });

	list_button->click();
	EXPECT_EQ(emissions, 1);
	EXPECT_EQ(received, ProjectToolbar::ListView);
	EXPECT_TRUE(list_button->isChecked());
	EXPECT_FALSE(tree_button->isChecked());
	EXPECT_FALSE(icon_button->isChecked());

	icon_button->click();
	EXPECT_EQ(emissions, 2);
	EXPECT_EQ(received, ProjectToolbar::IconView);
	EXPECT_TRUE(icon_button->isChecked());
	EXPECT_FALSE(list_button->isChecked());

	// SetView only checks the button; it does not re-emit ViewChanged
	toolbar.SetView(ProjectToolbar::TreeView);
	EXPECT_EQ(emissions, 2);
	EXPECT_TRUE(tree_button->isChecked());
	EXPECT_FALSE(icon_button->isChecked());
}
