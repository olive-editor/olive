#include "sourcetable.h"
#include "panels/project.h"

#include "project/footage.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/panels.h"
#include "playback/playback.h"
#include "project/undo.h"
#include "project/sequence.h"
#include "mainwindow.h"
#include "io/config.h"
#include "project/media.h"
#include "debug.h"

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

SourceTable::SourceTable(QWidget* parent) : QTreeView(parent) {
    editing_item = NULL;
	setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    rename_timer.setInterval(1000);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&rename_timer, SIGNAL(timeout()), this, SLOT(rename_interval()));
    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(item_click(const QModelIndex&)));
    //connect(this, SIGNAL(itemChanged(Media*,int)), this, SLOT(item_renamed(Media*)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));
}

void SourceTable::show_context_menu() {
    QMenu menu(this);

    QAction* import_action = menu.addAction("Import...");
    connect(import_action, SIGNAL(triggered(bool)), panel_project, SLOT(import_dialog()));

    QAction* new_folder_action = menu.addAction("New Folder...");
    connect(new_folder_action, SIGNAL(triggered(bool)), mainWindow, SLOT(on_actionFolder_triggered()));

    QModelIndexList selected_items = selectionModel()->selectedRows();

    if (selected_items.size() > 0) {
        Media* m = static_cast<Media*>(selected_items.at(0).internalPointer());

        if (selected_items.size() == 1) {
            // replace footage
            int type = m->get_type();
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
        for (int i=0;i<selected_items.size();i++) {
            if (m->get_type() != MEDIA_TYPE_SEQUENCE) {
				all_sequences = false;
            }
            if (m->get_type() != MEDIA_TYPE_FOOTAGE) {
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

        if (selected_items.size() == 1) {
			QAction* properties_action = menu.addAction("Properties...");
			connect(properties_action, SIGNAL(triggered(bool)), panel_project, SLOT(open_properties()));
		}
    }

	menu.exec(QCursor::pos());
}

void SourceTable::create_seq_from_selected() {
    QModelIndexList selected_items = selectionModel()->selectedRows();

    if (!selected_items.isEmpty()) {
        QVector<Media*> media_list;
        for (int i=0;i<selected_items.size();i++) {
            media_list.append(static_cast<Media*>(selected_items.at(i).internalPointer()));
		}

		ComboAction* ca = new ComboAction();
        Sequence* s = create_sequence_from_media(media_list);

		// add clips to it
        panel_timeline->create_ghosts_from_media(s, 0, media_list);
		panel_timeline->add_clips_from_ghosts(ca, s);

		panel_project->new_sequence(ca, s, true, NULL);
		undo_stack.push(ca);
	}
}

void SourceTable::reveal_in_browser() {
    QModelIndexList selected_items = selectionModel()->selectedRows();
    Footage* m = static_cast<Footage*>(selected_items.at(0).internalPointer());

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

void SourceTable::item_renamed(Media* item) {
    if (editing_item == item) {
        MediaRename* mr = new MediaRename(item, "idk");
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
        edit(editing_index);
        //editItem(editing_item, 0);
    }
}
void SourceTable::item_click(const QModelIndex& index) {
    if (selectionModel()->selectedRows().size() == 1 && index.column() == 0) {
        Media* m = static_cast<Media*>(index.internalPointer());
        if (editing_item == m) {
            rename_timer.start();
        } else {
            editing_item = m;
            editing_index = index;
        }
    }
}

void SourceTable::mousePressEvent(QMouseEvent* event) {
    stop_rename_timer();
    QTreeView::mousePressEvent(event);
}

void SourceTable::mouseDoubleClickEvent(QMouseEvent* e) {
    stop_rename_timer();
    QModelIndexList selected_items = selectionModel()->selectedRows();
    if (selected_items.size() == 0) {
		panel_project->import_dialog();
    } else if (selected_items.size() == 1) {
        Media* item = static_cast<Media*>(selected_items.at(0).internalPointer());
        switch (item->get_type()) {
		case MEDIA_TYPE_FOOTAGE:
            panel_footage_viewer->set_media(item);
            panel_footage_viewer->setFocus();
			break;
		case MEDIA_TYPE_SEQUENCE:
            undo_stack.push(new ChangeSequenceAction(item->to_sequence()));
			break;
		}
    }
}

void SourceTable::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void SourceTable::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

void SourceTable::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    const QModelIndex& drop_item = indexAt(event->pos());
    Media* m = static_cast<Media*>(drop_item.internalPointer());
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
                    && drop_item.isValid()
                    && m->get_type() == MEDIA_TYPE_FOOTAGE
					&& !QFileInfo(paths.at(0)).isDir()
                    && config.drop_on_media_to_replace
                    && QMessageBox::question(this, "Replace Media", "You dropped a file onto '" + m->get_name() + "'. Would you like to replace it with the dropped file?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
				replace = true;
                panel_project->replace_media(m, paths.at(0));
			}
			if (!replace) {
                QModelIndex parent;
                if (drop_item.isValid()) {
                    if (m->get_type() == MEDIA_TYPE_FOLDER) {
                        parent = drop_item;
					} else {
                        parent = drop_item.parent();
					}
				}
                if (parent.isValid()) setExpanded(parent, true);
                panel_project->process_file_list(false, paths, static_cast<Media*>(parent.internalPointer()), NULL);
			}
        }
        event->acceptProposedAction();
    } else {
		event->ignore();

        // dragging files within project
        // if we dragged to the root OR dragged to a folder
        if (!drop_item.isValid() || (drop_item.isValid() && m->get_type() == MEDIA_TYPE_FOLDER)) {
            QVector<Media*> move_items;
            QModelIndexList selected_items = selectionModel()->selectedRows();
            for (int i=0;i<selected_items.size();i++) {
                const QModelIndex& item = selected_items.at(i);
                const QModelIndex& parent = item.parent();
                Media* s = static_cast<Media*>(item.internalPointer());
                if (parent != drop_item && item != drop_item) {
					bool ignore = false;
                    if (parent.isValid()) {
						// if child belongs to a selected parent, assume the user is just moving the parent and ignore the child
                        QModelIndex par = parent;
                        while (par.isValid() && !ignore) {
							for (int j=0;j<selected_items.size();j++) {
								if (par == selected_items.at(j)) {
									ignore = true;
									break;
								}
							}
                            par = par.parent();
						}
					}
					if (!ignore) {
						move_items.append(s);
					}
				}
            }
            if (move_items.size() > 0) {
                MediaMove* mm = new MediaMove(this);
                mm->to = m;
                mm->items = move_items;
                undo_stack.push(mm);
            }
		}
    }
}
