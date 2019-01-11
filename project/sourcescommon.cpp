#include "sourcescommon.h"

#include "panels/panels.h"
#include "project/media.h"
#include "project/undo.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/footage.h"
#include "panels/viewer.h"
#include "io/config.h"
#include "mainwindow.h"

#include <QProcess>
#include <QMenu>
#include <QAbstractItemView>
#include <QMimeData>
#include <QMessageBox>
#include <QDesktopServices>

SourcesCommon::SourcesCommon(Project* parent) :
	project_parent(parent),
	editing_item(NULL)
{
	rename_timer.setInterval(1000);
	connect(&rename_timer, SIGNAL(timeout()), this, SLOT(rename_interval()));
}

void SourcesCommon::create_seq_from_selected() {
	if (!selected_items.isEmpty()) {
		QVector<Media*> media_list;
		for (int i=0;i<selected_items.size();i++) {
			media_list.append(project_parent->item_to_media(selected_items.at(i)));
		}

		ComboAction* ca = new ComboAction();
		Sequence* s = create_sequence_from_media(media_list);

		// add clips to it
		panel_timeline->create_ghosts_from_media(s, 0, media_list);
		panel_timeline->add_clips_from_ghosts(ca, s);

		project_parent->new_sequence(ca, s, true, NULL);
		undo_stack.push(ca);
	}
}

void SourcesCommon::show_context_menu(QWidget* parent, const QModelIndexList& items) {
	QMenu menu(parent);

	selected_items = items;

	QAction* import_action = menu.addAction("Import...");
	QObject::connect(import_action, SIGNAL(triggered(bool)), project_parent, SLOT(import_dialog()));

	QMenu* new_menu = menu.addMenu("New");
	mainWindow->make_new_menu(new_menu);

	if (items.size() > 0) {
		Media* m = project_parent->item_to_media(items.at(0));

		if (items.size() == 1) {
			// replace footage
			int type = m->get_type();
			if (type == MEDIA_TYPE_FOOTAGE) {
				QAction* replace_action = menu.addAction("Replace/Relink Media");
				QObject::connect(replace_action, SIGNAL(triggered(bool)), project_parent, SLOT(replace_selected_file()));

#if defined(Q_OS_WIN)
				QAction* reveal_in_explorer = menu.addAction("Reveal in Explorer");
#elif defined(Q_OS_MAC)
				QAction* reveal_in_explorer = menu.addAction("Reveal in Finder");
#else
				QAction* reveal_in_explorer = menu.addAction("Reveal in File Manager");
#endif
				QObject::connect(reveal_in_explorer, SIGNAL(triggered(bool)), this, SLOT(reveal_in_browser()));
			}
			if (type != MEDIA_TYPE_FOLDER) {
				QAction* replace_clip_media = menu.addAction("Replace Clips Using This Media");
				QObject::connect(replace_clip_media, SIGNAL(triggered(bool)), project_parent, SLOT(replace_clip_media()));
			}
		}

		// duplicate item
		bool all_sequences = true;
		bool all_footage = true;
		for (int i=0;i<items.size();i++) {
			if (m->get_type() != MEDIA_TYPE_SEQUENCE) {
				all_sequences = false;
			}
			if (m->get_type() != MEDIA_TYPE_FOOTAGE) {
				all_footage = false;
			}
		}

		// create sequence from
		QAction* create_seq_from = menu.addAction("Create Sequence With This Media");
		QObject::connect(create_seq_from, SIGNAL(triggered(bool)), this, SLOT(create_seq_from_selected()));

		// ONLY sequences are selected
		if (all_sequences) {
			// ONLY sequences are selected
			QAction* duplicate_action = menu.addAction("Duplicate");
			QObject::connect(duplicate_action, SIGNAL(triggered(bool)), project_parent, SLOT(duplicate_selected()));
		}

		// ONLY footage is selected
		if (all_footage) {
			QAction* delete_footage_from_sequences = menu.addAction("Delete All Clips Using This Media");
			QObject::connect(delete_footage_from_sequences, SIGNAL(triggered(bool)), project_parent, SLOT(delete_clips_using_selected_media()));
		}

		// delete media
		QAction* delete_action = menu.addAction("Delete");
		QObject::connect(delete_action, SIGNAL(triggered(bool)), project_parent, SLOT(delete_selected_media()));

		if (items.size() == 1) {
			QAction* properties_action = menu.addAction("Properties...");
			QObject::connect(properties_action, SIGNAL(triggered(bool)), project_parent, SLOT(open_properties()));
		}
	}

	menu.addSeparator();

	QAction* tree_view_action = menu.addAction("Tree View");
	connect(tree_view_action, SIGNAL(triggered(bool)), project_parent, SLOT(set_tree_view()));

	QAction* icon_view_action = menu.addAction("Icon View");
	connect(icon_view_action, SIGNAL(triggered(bool)), project_parent, SLOT(set_icon_view()));

	QAction* toolbar_action = menu.addAction("Show Toolbar");
	toolbar_action->setCheckable(true);
	toolbar_action->setChecked(project_parent->toolbar_widget->isVisible());
	connect(toolbar_action, SIGNAL(triggered(bool)), project_parent->toolbar_widget, SLOT(setVisible(bool)));

	menu.exec(QCursor::pos());
}

void SourcesCommon::mousePressEvent(QMouseEvent *) {
	stop_rename_timer();
}

void SourcesCommon::item_click(Media *m, const QModelIndex& index) {
	if (editing_item == m) {
		rename_timer.start();
	} else {
		editing_item = m;
		editing_index = index;
	}
}

void SourcesCommon::mouseDoubleClickEvent(QMouseEvent *e, const QModelIndexList& selected_items) {
	stop_rename_timer();
	if (selected_items.size() == 0) {
		project_parent->import_dialog();
	} else if (selected_items.size() == 1) {
		Media* item = project_parent->item_to_media(selected_items.at(0));
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

void SourcesCommon::dropEvent(QWidget* parent, QDropEvent *event, const QModelIndex& drop_item, const QModelIndexList& items) {
	const QMimeData* mimeData = event->mimeData();
	Media* m = project_parent->item_to_media(drop_item);
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
					&& QMessageBox::question(parent, "Replace Media", "You dropped a file onto '" + m->get_name() + "'. Would you like to replace it with the dropped file?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
				replace = true;
				project_parent->replace_media(m, paths.at(0));
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
				project_parent->process_file_list(paths, false, NULL, panel_project->item_to_media(parent));
			}
		}
		event->acceptProposedAction();
	} else {
		event->ignore();

		// dragging files within project
		// if we dragged to the root OR dragged to a folder
		if (!drop_item.isValid() || m->get_type() == MEDIA_TYPE_FOLDER) {
			QVector<Media*> move_items;
			for (int i=0;i<items.size();i++) {
				const QModelIndex& item = items.at(i);
				const QModelIndex& parent = item.parent();
				Media* s = project_parent->item_to_media(item);
				if (parent != drop_item && item != drop_item) {
					bool ignore = false;
					if (parent.isValid()) {
						// if child belongs to a selected parent, assume the user is just moving the parent and ignore the child
						QModelIndex par = parent;
						while (par.isValid() && !ignore) {
							for (int j=0;j<items.size();j++) {
								if (par == items.at(j)) {
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
				MediaMove* mm = new MediaMove();
				mm->to = m;
				mm->items = move_items;
				undo_stack.push(mm);
			}
		}
	}
}

void SourcesCommon::reveal_in_browser() {
	Media* media = project_parent->item_to_media(selected_items.at(0));
	Footage* m = media->to_footage();

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

void SourcesCommon::stop_rename_timer() {
	rename_timer.stop();
}

void SourcesCommon::rename_interval() {
	stop_rename_timer();
	if (view->hasFocus() && editing_item != NULL) {
		view->edit(editing_index);
	}
}

void SourcesCommon::item_renamed(Media* item) {
	if (editing_item == item) {
		MediaRename* mr = new MediaRename(item, "idk");
		undo_stack.push(mr);
		editing_item = NULL;
	}
}
