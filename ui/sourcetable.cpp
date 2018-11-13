#include "sourcetable.h"
#include "panels/project.h"

#include "io/media.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/panels.h"
#include "playback/playback.h"
#include "project/undo.h"
#include "project/sequence.h"
#include "mainwindow.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>

SourceTable::SourceTable(QWidget* parent) : QTreeWidget(parent) {
    editing_item = NULL;
	setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    rename_timer.setInterval(1000);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&rename_timer, SIGNAL(timeout()), this, SLOT(rename_interval()));
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(item_click(QTreeWidgetItem*,int)));
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(item_renamed(QTreeWidgetItem*)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));
}



void SourceTable::show_context_menu() {
    QMenu menu(this);

    if (selectedItems().size() == 0) {
        QAction* import_action = menu.addAction("Import...");
        connect(import_action, SIGNAL(triggered(bool)), panel_project, SLOT(import_dialog()));

		QAction* new_folder_action = menu.addAction("New Folder...");
		connect(new_folder_action, SIGNAL(triggered(bool)), mainWindow, SLOT(on_actionFolder_triggered()));
    } else {
        if (selectedItems().size() == 1) {
            // replace footage
			int type = get_type_from_tree(selectedItems().at(0));
			if (type == MEDIA_TYPE_FOOTAGE) {
				QAction* replace_action = menu.addAction("Replace/Relink Media");
				connect(replace_action, SIGNAL(triggered(bool)), panel_project, SLOT(replace_selected_file()));

#if defined(Q_OS_WIN)
				QAction* reveal_in_explorer = menu.addAction("Reveal in Explorer");
#elif defined(Q_OS_MAC)
				QAction* reveal_in_explorer = menu.addAction("Reveal in Finder");
#else
				QAction* reveal_in_explorer = menu.addAction("Reveal in File Manager");
#endif
				connect(reveal_in_explorer, SIGNAL(triggered(bool)), this, SLOT(reveal_in_browser()));
            }
			if (type != MEDIA_TYPE_FOLDER) {
				QAction* replace_clip_media = menu.addAction("Replace Clips Using This Media");
				connect(replace_clip_media, SIGNAL(triggered(bool)), panel_project, SLOT(replace_clip_media()));
			}
        }

        // duplicate item
        bool all_sequences = true;
		bool all_footage = true;
        for (int i=0;i<selectedItems().size();i++) {
            if (get_type_from_tree(selectedItems().at(i)) != MEDIA_TYPE_SEQUENCE) {
				all_sequences = false;
            }
			if (get_type_from_tree(selectedItems().at(i)) != MEDIA_TYPE_FOOTAGE) {
				all_footage = false;
			}
        }

		// create sequence from
		QAction* create_seq_from = menu.addAction("Create Sequence With This Media");
		connect(create_seq_from, SIGNAL(triggered(bool)), this, SLOT(create_seq_from_selected()));

		// ONLY sequences are selected
        if (all_sequences) {
			// ONLY sequences are selected
            QAction* duplicate_action = menu.addAction("Duplicate");
            connect(duplicate_action, SIGNAL(triggered(bool)), panel_project, SLOT(duplicate_selected()));
		}

		// ONLY footage is selected
		if (all_footage) {
			QAction* delete_footage_from_sequences = menu.addAction("Delete All Clips Using This Media");
			connect(delete_footage_from_sequences, SIGNAL(triggered(bool)), panel_project, SLOT(delete_clips_using_selected_media()));
		}

        // delete media
        QAction* delete_action = menu.addAction("Delete");
        connect(delete_action, SIGNAL(triggered(bool)), panel_project, SLOT(delete_selected_media()));

		if (selectedItems().size() == 1) {
			QAction* properties_action = menu.addAction("Properties...");
			connect(properties_action, SIGNAL(triggered(bool)), panel_project, SLOT(open_properties()));
		}
    }

	menu.exec(QCursor::pos());
}

void SourceTable::create_seq_from_selected() {
	if (!selectedItems().isEmpty()) {
		QVector<void*> media_list;
		QVector<int> type_list;
		for (int i=0;i<selectedItems().size();i++) {
			QTreeWidgetItem* item = selectedItems().at(i);
			media_list.append(get_media_from_tree(item));
			type_list.append(get_type_from_tree(item));
		}

		ComboAction* ca = new ComboAction();
		Sequence* s = create_sequence_from_media(media_list, type_list);

		// add clips to it
		panel_timeline->create_ghosts_from_media(s, 0, media_list, type_list);
		panel_timeline->add_clips_from_ghosts(ca, s);

		panel_project->new_sequence(ca, s, true, NULL);
		undo_stack.push(ca);
	}
}

void SourceTable::reveal_in_browser() {
	Media* m = get_footage_from_tree(selectedItems().at(0));

#if defined(Q_OS_WIN)
	QStringList args;
	args << "/select," << QDir::toNativeSeparators(m->url);
	QProcess::startDetached("explorer", args);
#elif defined(Q_OS_MAC)
	QStringList args;
	args << "-e";
	args << "tell application \"Finder\"";
	args << "-e";
	args << "activate";
	args << "-e";
	args << "select POSIX file \""+m->url+"\"";
	args << "-e";
	args << "end tell";
	QProcess::startDetached("osascript", args);
#else
	QDesktopServices::openUrl(QUrl::fromLocalFile(m->url.left(m->url.lastIndexOf('/'))));
#endif
}

void SourceTable::item_renamed(QTreeWidgetItem* item) {
    if (editing_item == item) {
        MediaRename* mr = new MediaRename();
        mr->from = editing_item_name;
        mr->item = editing_item;
        mr->to = editing_item->text(0);
        undo_stack.push(mr);
        editing_item = NULL;
    }
}

void SourceTable::stop_rename_timer() {
    rename_timer.stop();
}

void SourceTable::rename_interval() {
    stop_rename_timer();
    if (hasFocus() && editing_item != NULL) {
        editing_item_name = editing_item->text(0);
        editItem(editing_item, 0);
    }
}
void SourceTable::item_click(QTreeWidgetItem* item, int column) {
	if (column == 0 && selectedItems().size() == 1) {
        editing_item = item;
        rename_timer.start();
    }
}

void SourceTable::mousePressEvent(QMouseEvent* event) {
    stop_rename_timer();
    QTreeWidget::mousePressEvent(event);
}

void SourceTable::mouseDoubleClickEvent(QMouseEvent* ) {
    stop_rename_timer();
	if (selectedItems().count() == 0) {
		panel_project->import_dialog();
	} else if (selectedItems().count() == 1) {
        QTreeWidgetItem* item = selectedItems().at(0);
		switch (get_type_from_tree(item)) {
		case MEDIA_TYPE_FOOTAGE:
			panel_footage_viewer->set_media(get_type_from_tree(item), get_media_from_tree(item));
			break;
		case MEDIA_TYPE_SEQUENCE:
			undo_stack.push(new ChangeSequenceAction(get_sequence_from_tree(item)));
			break;
		}
    }
}

void SourceTable::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeWidget::dragEnterEvent(event);
    }
}

void SourceTable::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeWidget::dragMoveEvent(event);
    }
}

void SourceTable::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
	QTreeWidgetItem* drop_item = itemAt(event->pos());
    if (mimeData->hasUrls()) {
        // drag files in from outside
        QList<QUrl> urls = mimeData->urls();
        if (!urls.isEmpty()) {			
            QStringList paths;
			for (int i=0;i<urls.size();i++) {
                paths.append(urls.at(i).toLocalFile());
            }
			bool replace = false;
			if (urls.size() == 1
					&& drop_item != NULL
					&& get_type_from_tree(drop_item) == MEDIA_TYPE_FOOTAGE
					&& !QFileInfo(paths.at(0)).isDir()
					&& QMessageBox::question(this, "Replace Media", "You dropped a file onto '" + drop_item->text(0) + "'. Would you like to replace it with the dropped file?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
				replace = true;
				panel_project->replace_media(drop_item, paths.at(0));
			}
			if (!replace) {
				QTreeWidgetItem* parent = NULL;
				if (drop_item != NULL) {
					if (get_type_from_tree(drop_item) == MEDIA_TYPE_FOLDER) {
						parent = drop_item;
					} else {
						parent = drop_item->parent();
					}
				}
				if (parent != NULL) parent->setExpanded(true);
				panel_project->process_file_list(false, paths, parent, NULL);
			}
        }
        event->acceptProposedAction();
    } else {
		event->ignore();

        // dragging files within project
        // if we dragged to the root OR dragged to a folder
        if (drop_item == NULL || (drop_item != NULL && get_type_from_tree(drop_item) == MEDIA_TYPE_FOLDER)) {
			QVector<QTreeWidgetItem*> move_items;
            QList<QTreeWidgetItem*> selected_items = selectedItems();
            for (int i=0;i<selected_items.size();i++) {
                QTreeWidgetItem* s = selected_items.at(i);
				if (s->parent() != drop_item && s != drop_item) {
					bool ignore = false;
					if (s->parent() != NULL) {
						// if child belongs to a selected parent, assume the user is just moving the parent and ignore the child
						QTreeWidgetItem* par = s->parent();
						while (par != NULL && !ignore) {
							for (int j=0;j<selected_items.size();j++) {
								if (par == selected_items.at(j)) {
									ignore = true;
									break;
								}
							}
							par = par->parent();
						}
					}
					if (!ignore) {
						move_items.append(s);
					}
				}
            }
            if (move_items.size() > 0) {
                MediaMove* mm = new MediaMove(this);
                mm->to = drop_item;
                mm->items = move_items;
                undo_stack.push(mm);
            }
		}
    }
}
