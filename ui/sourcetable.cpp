#include "sourcetable.h"
#include "panels/project.h"

#include "io/media.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/panels.h"
#include "playback/playback.h"
#include "project/undo.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

SourceTable::SourceTable(QWidget* parent) : QTreeWidget(parent) {
    editing_item = NULL;
    sortByColumn(0, Qt::AscendingOrder);
    rename_timer.setInterval(500);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&rename_timer, SIGNAL(timeout()), this, SLOT(rename_interval()));
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(item_click(QTreeWidgetItem*,int)));
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(item_renamed(QTreeWidgetItem*)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));
}

void SourceTable::show_context_menu(const QPoint& pos) {
    QMenu menu(this);

    if (selectedItems().size() == 0) {
        QAction* import_action = menu.addAction("Import...");
        connect(import_action, SIGNAL(triggered(bool)), panel_project, SLOT(import_dialog()));
    } else {
        if (selectedItems().size() == 1) {
            // replace footage
			int type = get_type_from_tree(selectedItems().at(0));
			if (type == MEDIA_TYPE_FOOTAGE) {
				QAction* replace_action = menu.addAction("Replace/Relink Media");
				connect(replace_action, SIGNAL(triggered(bool)), panel_project, SLOT(replace_selected_file()));
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
    }

    menu.exec(mapToGlobal(pos));
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
    if (column == 0) {
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
        if (get_type_from_tree(item) == MEDIA_TYPE_SEQUENCE) {
            TimelineAction* ta = new TimelineAction();
            ta->change_sequence(get_sequence_from_tree(item));
            undo_stack.push(ta);
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
				if (drop_item != NULL && get_type_from_tree(drop_item) == MEDIA_TYPE_FOLDER) {
					parent = drop_item;
				}
				panel_project->process_file_list(false, paths, parent, NULL);
			}
        }
        event->acceptProposedAction();
    } else {
        // dragging files within project
        QVector<QTreeWidgetItem*> move_items;
        // if we dragged to the root OR dragged to a folder
        if (drop_item == NULL || (drop_item != NULL && get_type_from_tree(drop_item) == MEDIA_TYPE_FOLDER)) {
            QList<QTreeWidgetItem*> selected_items = selectedItems();
            for (int i=0;i<selected_items.size();i++) {
                QTreeWidgetItem* s = selected_items.at(i);
                bool ignore = false;
                if (s->parent() != NULL) {
                    // if child belongs to a selected parent, assume the user is just moving the parent and ignore the child
                    QTreeWidgetItem* par = s->parent();
                    while (par != NULL) {
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
            if (move_items.size() > 0) {
                MediaMove* mm = new MediaMove(this);
                mm->to = drop_item;
                mm->items = move_items;
                undo_stack.push(mm);
            }
        }
    }
}
