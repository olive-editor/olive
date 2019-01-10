#include "project.h"
#include "project/footage.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "playback/playback.h"
#include "project/effect.h"
#include "project/transition.h"
#include "panels/timeline.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "io/previewgenerator.h"
#include "project/undo.h"
#include "mainwindow.h"
#include "io/config.h"
#include "playback/cacher.h"
#include "dialogs/replaceclipmediadialog.h"
#include "panels/effectcontrols.h"
#include "dialogs/newsequencedialog.h"
#include "dialogs/mediapropertiesdialog.h"
#include "dialogs/loaddialog.h"
#include "io/clipboard.h"
#include "project/media.h"
#include "ui/sourcetable.h"
#include "ui/sourceiconview.h"
#include "project/sourcescommon.h"
#include "debug.h"

#include <QApplication>
#include <QFileDialog>
#include <QString>
#include <QVariant>
#include <QCharRef>
#include <QMessageBox>
#include <QDropEvent>
#include <QMimeData>
#include <QPushButton>
#include <QInputDialog>
#include <QSortFilterProxyModel>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QMenu>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}

#define MAXIMUM_RECENT_PROJECTS 10

ProjectModel project_model;

QString autorecovery_filename;
QString project_url = "";
QStringList recent_projects;
QString recent_proj_file;

Project::Project(QWidget *parent) :
	QDockWidget(parent)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	QWidget* dockWidgetContents = new QWidget();
	QVBoxLayout* verticalLayout = new QVBoxLayout(dockWidgetContents);
	verticalLayout->setContentsMargins(0, 0, 0, 0);
	verticalLayout->setSpacing(0);

	setWidget(dockWidgetContents);

	sources_common = new SourcesCommon(this);

	sorter = new QSortFilterProxyModel(this);
	sorter->setSourceModel(&project_model);

	// optional toolbar
	toolbar_widget = new QWidget();
	toolbar_widget->setVisible(config.show_project_toolbar);
	QHBoxLayout* toolbar = new QHBoxLayout();
	toolbar->setMargin(0);
	toolbar->setSpacing(0);
	toolbar_widget->setLayout(toolbar);

	QPushButton* toolbar_new = new QPushButton("New");
	toolbar_new->setIcon(QIcon(":/icons/tri-down.png"));
	toolbar_new->setIconSize(QSize(8, 8));
	toolbar_new->setToolTip("New");
	connect(toolbar_new, SIGNAL(clicked(bool)), this, SLOT(make_new_menu()));
	toolbar->addWidget(toolbar_new);

	QPushButton* toolbar_open = new QPushButton("Open");
	toolbar_open->setToolTip("Open Project");
	connect(toolbar_open, SIGNAL(clicked(bool)), mainWindow, SLOT(open_project()));
	toolbar->addWidget(toolbar_open);

	QPushButton* toolbar_save = new QPushButton("Save");
	toolbar_save->setToolTip("Save Project");
	connect(toolbar_save, SIGNAL(clicked(bool)), mainWindow, SLOT(save_project()));
	toolbar->addWidget(toolbar_save);

	QPushButton* toolbar_undo = new QPushButton("Undo");
	toolbar_undo->setToolTip("Undo");
	connect(toolbar_undo, SIGNAL(clicked(bool)), mainWindow, SLOT(undo()));
	toolbar->addWidget(toolbar_undo);

	QPushButton* toolbar_redo = new QPushButton("Redo");
	toolbar_redo->setToolTip("Redo");
	connect(toolbar_redo, SIGNAL(clicked(bool)), mainWindow, SLOT(redo()));
	toolbar->addWidget(toolbar_redo);

	toolbar->addStretch();

	QPushButton* toolbar_tree_view = new QPushButton("Tree View");
	toolbar_tree_view->setToolTip("Tree View");
	connect(toolbar_tree_view, SIGNAL(clicked(bool)), this, SLOT(set_tree_view()));
	toolbar->addWidget(toolbar_tree_view);

	QPushButton* toolbar_icon_view = new QPushButton("Icon View");
	toolbar_icon_view->setToolTip("Icon View");
	connect(toolbar_icon_view, SIGNAL(clicked(bool)), this, SLOT(set_icon_view()));
	toolbar->addWidget(toolbar_icon_view);

	verticalLayout->addWidget(toolbar_widget);

	// tree view
	tree_view = new SourceTable(dockWidgetContents);
	tree_view->project_parent = this;
	tree_view->setModel(sorter);
	verticalLayout->addWidget(tree_view);

	// icon view
	icon_view_container = new QWidget();

	QVBoxLayout* icon_view_container_layout = new QVBoxLayout();
	icon_view_container_layout->setMargin(0);
	icon_view_container_layout->setSpacing(0);
	icon_view_container->setLayout(icon_view_container_layout);

	QHBoxLayout* icon_view_controls = new QHBoxLayout();
	icon_view_controls->setMargin(0);
	icon_view_controls->setSpacing(0);

	QIcon directory_up_button;
	directory_up_button.addFile(":/icons/dirup.png", QSize(), QIcon::Normal);
	directory_up_button.addFile(":/icons/dirup-disabled.png", QSize(), QIcon::Disabled);

	directory_up = new QPushButton();
	directory_up->setIcon(directory_up_button);
	directory_up->setEnabled(false);
	icon_view_controls->addWidget(directory_up);

	icon_view_controls->addStretch();

	QSlider* icon_size_slider = new QSlider(Qt::Horizontal);
	icon_size_slider->setMinimum(16);
	icon_size_slider->setMaximum(120);
	icon_view_controls->addWidget(icon_size_slider);
	connect(icon_size_slider, SIGNAL(valueChanged(int)), this, SLOT(set_icon_view_size(int)));

	icon_view_container_layout->addLayout(icon_view_controls);

	icon_view = new SourceIconView(dockWidgetContents);
	icon_view->project_parent = this;
	icon_view->setModel(sorter);
	icon_view->setIconSize(QSize(100, 100));
	icon_view->setViewMode(QListView::IconMode);
	icon_view->setUniformItemSizes(true);
	icon_view_container_layout->addWidget(icon_view);

	icon_size_slider->setValue(icon_view->iconSize().height());

	verticalLayout->addWidget(icon_view_container);

	connect(directory_up, SIGNAL(clicked(bool)), this, SLOT(go_up_dir()));
	connect(icon_view, SIGNAL(changed_root()), this, SLOT(set_up_dir_enabled()));

	//retranslateUi(Project);
	setWindowTitle(QApplication::translate("Project", "Project", nullptr));

	update_view_type();
}

Project::~Project() {
	delete sorter;
}

QString Project::get_next_sequence_name(QString start) {
	if (start.isEmpty()) start = "Sequence";

	int n = 1;
	bool found = true;
	QString name;
	while (found) {
		found = false;
		name = start + " ";
		if (n < 10) {
			name += "0";
		}
		name += QString::number(n);
		for (int i=0;i<project_model.childCount();i++) {
			if (QString::compare(project_model.child(i)->get_name(), name, Qt::CaseInsensitive) == 0) {
				found = true;
				n++;
				break;
			}
		}
	}
	return name;
}

Sequence* create_sequence_from_media(QVector<Media*>& media_list) {
	Sequence* s = new Sequence();

	s->name = panel_project->get_next_sequence_name();

	// shitty hardcoded default values
	s->width = 1920;
	s->height = 1080;
	s->frame_rate = 29.97;
	s->audio_frequency = 48000;
	s->audio_layout = 3;

	bool got_video_values = false;
	bool got_audio_values = false;
	for (int i=0;i<media_list.size();i++) {
		Media* media = media_list.at(i);
		switch (media->get_type()) {
		case MEDIA_TYPE_FOOTAGE:
		{
			Footage* m = media->to_footage();
			if (m->ready) {
				if (!got_video_values) {
					for (int j=0;j<m->video_tracks.size();j++) {
						const FootageStream& ms = m->video_tracks.at(j);
						s->width = ms.video_width;
						s->height = ms.video_height;
						if (ms.video_frame_rate != 0) {
							s->frame_rate = ms.video_frame_rate * m->speed;

							if (ms.video_interlacing != VIDEO_PROGRESSIVE) s->frame_rate *= 2;

							// only break with a decent frame rate, otherwise there may be a better candidate
							got_video_values = true;
							break;
						}
					}
				}
				if (!got_audio_values) {
					for (int j=0;j<m->audio_tracks.size();j++) {
						const FootageStream& ms = m->audio_tracks.at(j);
						s->audio_frequency = ms.audio_frequency;
						got_audio_values = true;
						break;
					}
				}
			}
		}
			break;
		case MEDIA_TYPE_SEQUENCE:
		{
			Sequence* seq = media->to_sequence();
			s->width = seq->width;
			s->height = seq->height;
			s->frame_rate = seq->frame_rate;
			s->audio_frequency = seq->audio_frequency;
			s->audio_layout = seq->audio_layout;

			got_video_values = true;
			got_audio_values = true;
		}
			break;
		}
		if (got_video_values && got_audio_values) break;
	}

	return s;
}

void Project::duplicate_selected() {
	QModelIndexList items = get_current_selected();
	bool duped = false;
	ComboAction* ca = new ComboAction();
	for (int j=0;j<items.size();j++) {
		dout << "duplicate called";
		Media* i = item_to_media(items.at(j));
		if (i->get_type() == MEDIA_TYPE_SEQUENCE) {
			new_sequence(ca, i->to_sequence()->copy(), false, item_to_media(items.at(j).parent()));
			duped = true;
		}
	}
	if (duped) {
		undo_stack.push(ca);
	} else {
		delete ca;
	}
}

void Project::replace_selected_file() {
	QModelIndexList selected_items = get_current_selected();
	if (selected_items.size() == 1) {
		Media* item = item_to_media(selected_items.at(0));
		if (item->get_type() == MEDIA_TYPE_FOOTAGE) {
			replace_media(item, 0);
		}
	}
}

void Project::replace_media(Media* item, QString filename) {
	if (filename.isEmpty()) {
		filename = QFileDialog::getOpenFileName(this, "Replace '" + item->get_name() + "'", "", "All Files (*)");
	}
	if (!filename.isEmpty()) {
		ReplaceMediaCommand* rmc = new ReplaceMediaCommand(item, filename);
		undo_stack.push(rmc);
	}
}

void Project::replace_clip_media() {
	if (sequence == NULL) {
		QMessageBox::critical(this, "No active sequence", "No sequence is active, please open the sequence you want to replace clips from.", QMessageBox::Ok);
	} else {
		QModelIndexList selected_items = get_current_selected();
		if (selected_items.size() == 1) {
			Media* item = item_to_media(selected_items.at(0));
			if (item->get_type() == MEDIA_TYPE_SEQUENCE && sequence == item->to_sequence()) {
				QMessageBox::critical(this, "Active sequence selected", "You cannot insert a sequence into itself, so no clips of this media would be in this sequence.", QMessageBox::Ok);
			} else {
				ReplaceClipMediaDialog dialog(this, item);
				dialog.exec();
			}
		}
	}
}

void Project::open_properties() {
	QModelIndexList selected_items = get_current_selected();
	if (selected_items.size() == 1) {
		Media* item = item_to_media(selected_items.at(0));
		switch (item->get_type()) {
		case MEDIA_TYPE_FOOTAGE:
		{
			MediaPropertiesDialog mpd(this, item);
			mpd.exec();
		}
			break;
		case MEDIA_TYPE_SEQUENCE:
		{
			NewSequenceDialog nsd(this, item);
			nsd.exec();
		}
			break;
		default:
		{
			// fall back to renaming
			QString new_name = QInputDialog::getText(this, "Rename '" + item->get_name() + "'", "Enter new name:", QLineEdit::Normal, item->get_name());
			if (!new_name.isEmpty()) {
				MediaRename* mr = new MediaRename(item, new_name);
				undo_stack.push(mr);
			}
		}
		}
	}
}

Media* Project::new_sequence(ComboAction *ca, Sequence *s, bool open, Media* parent) {
	if (parent == NULL) parent = project_model.get_root();
	Media* item = new Media(parent);
	item->set_sequence(s);

	if (ca != NULL) {
		ca->append(new NewSequenceCommand(item, parent));
		if (open) ca->append(new ChangeSequenceAction(s));
	} else {
		project_model.appendChild(NULL, item);
		if (open) set_sequence(s);
	}
	return item;
}

QString Project::get_file_name_from_path(const QString& path) {
	return path.mid(path.lastIndexOf('/')+1);
}

/*Media* Project::new_item() {
	Media* item = new Media(0);
	//item->setFlags(item->flags() | Qt::ItemIsEditable);
	return item;
}*/

bool Project::is_focused() {
	return tree_view->hasFocus() || icon_view->hasFocus();
}

Media* Project::new_folder(QString name) {
	Media* item = new Media(0);
	item->set_folder();
	return item;
}

Media *Project::item_to_media(const QModelIndex &index) {
	return static_cast<Media*>(sorter->mapToSource(index).internalPointer());
//    return static_cast<Media*>(index.internalPointer());
}

void Project::get_all_media_from_table(QList<Media*>& items, QList<Media*>& list, int search_type) {
	for (int i=0;i<items.size();i++) {
		Media* item = items.at(i);
		if (item->get_type() == MEDIA_TYPE_FOLDER) {
			QList<Media*> children;
			for (int j=0;j<item->childCount();j++) {
				children.append(item->child(j));
			}
			get_all_media_from_table(children, list, search_type);
		} else if (search_type == item->get_type() || search_type == -1) {
			list.append(item);
		}
	}
}

bool delete_clips_in_clipboard_with_media(ComboAction* ca, Media* m) {
	int delete_count = 0;
	if (clipboard_type == CLIPBOARD_TYPE_CLIP) {
		for (int i=0;i<clipboard.size();i++) {
			Clip* c = static_cast<Clip*>(clipboard.at(i));
			if (c->media == m) {
				ca->append(new RemoveClipsFromClipboard(i-delete_count));
				delete_count++;
			}
		}
	}
	return (delete_count > 0);
}

void Project::delete_selected_media() {
	ComboAction* ca = new ComboAction();
	QModelIndexList selected_items = get_current_selected();
	QList<Media*> items;
	for (int i=0;i<selected_items.size();i++) {
		items.append(item_to_media(selected_items.at(i)));
	}
	bool remove = true;
	bool redraw = false;

	// check if media is in use
	QVector<Media*> parents;
	QList<Media*> sequence_items;
	QList<Media*> all_top_level_items;
	for (int i=0;i<project_model.childCount();i++) {
		all_top_level_items.append(project_model.child(i));
	}
	get_all_media_from_table(all_top_level_items, sequence_items, MEDIA_TYPE_SEQUENCE); // find all sequences in project
	if (sequence_items.size() > 0) {
		QList<Media*> media_items;
		get_all_media_from_table(items, media_items, MEDIA_TYPE_FOOTAGE);
		for (int i=0;i<media_items.size();i++) {
			Media* item = media_items.at(i);
			Footage* media = item->to_footage();
			bool confirm_delete = false;
			for (int j=0;j<sequence_items.size();j++) {
				Sequence* s = sequence_items.at(j)->to_sequence();
				for (int k=0;k<s->clips.size();k++) {
					Clip* c = s->clips.at(k);
					if (c != NULL && c->media == item) {
						if (!confirm_delete) {
							// we found a reference, so we know we'll need to ask if the user wants to delete it
							QMessageBox confirm(this);
							confirm.setWindowTitle("Delete media in use?");
							confirm.setText("The media '" + media->name + "' is currently used in '" + s->name + "'. Deleting it will remove all instances in the sequence. Are you sure you want to do this?");
							QAbstractButton* yes_button = confirm.addButton(QMessageBox::Yes);
							QAbstractButton* skip_button = NULL;
							if (items.size() > 1) skip_button = confirm.addButton("Skip", QMessageBox::NoRole);
							QAbstractButton* abort_button = confirm.addButton(QMessageBox::Cancel);
							confirm.exec();
							if (confirm.clickedButton() == yes_button) {
								// remove all clips referencing this media
								confirm_delete = true;
								redraw = true;
							} else if (confirm.clickedButton() == skip_button) {
								// remove media item and any folders containing it from the remove list
								Media* parent = item;
								while (parent != NULL) {
									parents.append(parent);

									// re-add item's siblings
									for (int m=0;m<parent->childCount();m++) {
										Media* child = parent->child(m);
										bool found = false;
										for (int n=0;n<items.size();n++) {
											if (items.at(n) == child) {
												found = true;
												break;
											}
										}
										if (!found) {
											items.append(child);
										}
									}

									parent = parent->parentItem();
								}

								j = sequence_items.size();
								k = s->clips.size();
							} else if (confirm.clickedButton() == abort_button) {
								// break out of loop
								i = media_items.size();
								j = sequence_items.size();
								k = s->clips.size();

								remove = false;
							}
						}
						if (confirm_delete) {
							ca->append(new DeleteClipAction(s, k));
						}
					}
				}
			}
			if (confirm_delete) {
				delete_clips_in_clipboard_with_media(ca, item);
			}
		}
	}

	// remove
	if (remove) {
		panel_effect_controls->clear_effects(true);
        if (sequence != NULL) sequence->selections.clear();

		// remove media and parents
		for (int m=0;m<parents.size();m++) {
			for (int l=0;l<items.size();l++) {
				if (items.at(l) == parents.at(m)) {
					items.removeAt(l);
					l--;
				}
			}
		}

		for (int i=0;i<items.size();i++) {
			ca->append(new DeleteMediaCommand(items.at(i)));

			if (items.at(i)->get_type() == MEDIA_TYPE_SEQUENCE) {
				redraw = true;

				Sequence* s = items.at(i)->to_sequence();

				if (s == sequence) {
					ca->append(new ChangeSequenceAction(NULL));
				}

				if (s == panel_footage_viewer->seq) {
					panel_footage_viewer->set_media(NULL);
				}
			} else if (items.at(i)->get_type() == MEDIA_TYPE_FOOTAGE) {
				if (panel_footage_viewer->seq != NULL) {
					for (int j=0;j<panel_footage_viewer->seq->clips.size();j++) {
						Clip* c = panel_footage_viewer->seq->clips.at(j);
						if (c != NULL) {
							if (c->media == items.at(i)->to_object()) {
								panel_footage_viewer->set_media(NULL);
							}
							break;
						}
					}
				}
			}
		}
		undo_stack.push(ca);

		// redraw clips
		if (redraw) {
			update_ui(true);
		}
	} else {
		delete ca;
	}
}

void Project::start_preview_generator(Media* item, bool replacing) {
	// set up throbber animation
	MediaThrobber* throbber = new MediaThrobber(item);
	throbber->moveToThread(QApplication::instance()->thread());
	item->throbber = throbber;
	QMetaObject::invokeMethod(throbber, "start", Qt::QueuedConnection);

	PreviewGenerator* pg = new PreviewGenerator(item, item->to_footage(), replacing);
	item->to_footage()->preview_gen = pg;
	connect(pg, SIGNAL(set_icon(int, bool)), throbber, SLOT(stop(int, bool)));
	pg->start(QThread::LowPriority);
}

void Project::process_file_list(QStringList& files, bool recursive, Media* replace, Media* parent) {
	bool imported = false;

	QVector<QString> image_sequence_urls;
	QVector<bool> image_sequence_importassequence;
	QStringList image_sequence_formats = config.img_seq_formats.split("|");

	if (!recursive) last_imported_media.clear();

	bool create_undo_action = (!recursive && replace == NULL);
	ComboAction* ca;
	if (create_undo_action) ca = new ComboAction();

	for (int i=0;i<files.size();i++) {
		if (QFileInfo(files.at(i)).isDir()) {
			QString folder_name = get_file_name_from_path(files.at(i));
			Media* folder = new_folder(folder_name);

			QDir directory(files.at(i));
			directory.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

			QFileInfoList subdir_files = directory.entryInfoList();
			QStringList subdir_filenames;

			for (int j=0;j<subdir_files.size();j++) {
				subdir_filenames.append(subdir_files.at(j).filePath());
			}

			process_file_list(subdir_filenames, true, NULL, folder);

			if (create_undo_action) {
				ca->append(new AddMediaCommand(folder, parent));
			} else {
				project_model.appendChild(parent, folder);
			}

			imported = true;
		} else if (!files.at(i).isEmpty()) {
			QString file(files.at(i));
			bool skip = false;

			/* Heuristic to determine whether file is part of an image sequence */

			// check file extension (assume it's not a

			int lastcharindex = file.lastIndexOf(".");
			bool found = true;
			if (lastcharindex != -1 && lastcharindex > file.lastIndexOf('/')) {
				// image_sequence_formats
				found = false;
				QString ext = file.mid(lastcharindex+1);
				for (int j=0;j<image_sequence_formats.size();j++) {
					if (ext == image_sequence_formats.at(j)) {
						found = true;
						break;
					}
				}
			} else {
				lastcharindex = file.length();
			}

			if (lastcharindex == 0) lastcharindex++;

			if (found && file[lastcharindex-1].isDigit()) {
				bool is_img_sequence = false;

				// how many digits are in the filename?
				int digit_count = 0;
				int digit_test = lastcharindex-1;
				while (file[digit_test].isDigit()) {
					digit_count++;
					digit_test--;
				}

				// retrieve number from filename
				digit_test++;
				int file_number = file.mid(digit_test, digit_count).toInt();

				// Check if there are files with the same filename but just different numbers
				if (QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number-1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))
						|| QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number+1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))) {
					is_img_sequence = true;
				}

				if (is_img_sequence) {
					// get the URL that we would pass to FFmpeg to force it to read the image as a sequence
					QString new_filename = file.left(digit_test) + "%" + QString("%1").arg(digit_count, 2, 10, QChar('0')) + "d" + file.mid(lastcharindex);

					// add image sequence url to a vector in case the user imported several files that
					// we're interpreting as a possible sequence
					found = false;
					for (int i=0;i<image_sequence_urls.size();i++) {
						if (image_sequence_urls.at(i) == new_filename) {
							// either SKIP if we're importing as a sequence, or leave it if we aren't
							if (image_sequence_importassequence.at(i)) {
								skip = true;
							}
							found = true;
							break;
						}
					}
					if (!found) {
						image_sequence_urls.append(new_filename);
						if (QMessageBox::question(this, "Image sequence detected", "The file '" + file + "' appears to be part of an image sequence. Would you like to import it as such?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
							file = new_filename;
							image_sequence_importassequence.append(true);
						} else {
							image_sequence_importassequence.append(false);
						}
					}
				}
			}

			if (!skip) {
				Media* item;
				Footage* m;

				if (replace != NULL) {
					item = replace;
					m = replace->to_footage();
					m->reset();
				} else {
					item = new Media(parent);
					m = new Footage();
				}

				m->using_inout = false;
				m->url = file;
				m->name = get_file_name_from_path(files.at(i));

				item->set_footage(m);

				// generate waveform/thumbnail in another thread
				start_preview_generator(item, replace != NULL);

				last_imported_media.append(item);

				if (replace == NULL) {
					if (create_undo_action) {
						ca->append(new AddMediaCommand(item, parent));
					} else {
						project_model.appendChild(parent, item);
					}
				}

				imported = true;
			}
		}
	}
	if (create_undo_action) {
		if (imported) {
			undo_stack.push(ca);
		} else {
			delete ca;
		}
	}
}

Media* Project::get_selected_folder() {
	// if one item is selected and it's a folder, return it
	QModelIndexList selected_items = get_current_selected();
	if (selected_items.size() == 1) {
		Media* m = item_to_media(selected_items.at(0));
		if (m->get_type() == MEDIA_TYPE_FOLDER) return m;
	}
	return NULL;
}

bool Project::reveal_media(Media *media, QModelIndex parent) {
	for (int i=0;i<project_model.rowCount(parent);i++) {
		const QModelIndex& item = project_model.index(i, 0, parent);
		Media* m = project_model.getItem(item);

		if (m->get_type() == MEDIA_TYPE_FOLDER) {
			if (reveal_media(media, item)) return true;
		} else if (m == media) {
			// expand all folders leading to this media
			QModelIndex sorted_index = sorter->mapFromSource(item);

			QModelIndex hierarchy = sorted_index.parent();

			if (config.project_view_type == PROJECT_VIEW_TREE) {
				while (hierarchy.isValid()) {
					tree_view->setExpanded(hierarchy, true);
					hierarchy = hierarchy.parent();
				}

				// select item
				tree_view->selectionModel()->select(sorted_index, QItemSelectionModel::Select);
			} else if (config.project_view_type == PROJECT_VIEW_ICON) {
				icon_view->setRootIndex(hierarchy);
				icon_view->selectionModel()->select(sorted_index, QItemSelectionModel::Select);
				set_up_dir_enabled();
			}

			return true;
		}
	}

	return false;
}

void Project::import_dialog() {
	QFileDialog fd(this, "Import media...", "", "All Files (*)");
	fd.setFileMode(QFileDialog::ExistingFiles);

	if (fd.exec()) {
		QStringList files = fd.selectedFiles();
		process_file_list(files, false, NULL, get_selected_folder());
	}
}

void Project::delete_clips_using_selected_media() {
	if (sequence == NULL) {
		QMessageBox::critical(this, "No active sequence", "No sequence is active, please open the sequence you want to delete clips from.", QMessageBox::Ok);
	} else {
		ComboAction* ca = new ComboAction();
		bool deleted = false;
		QModelIndexList items = get_current_selected();
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* c = sequence->clips.at(i);
			if (c != NULL) {
				for (int j=0;j<items.size();j++) {
					Media* m = item_to_media(items.at(j));
					if (c->media == m) {
						ca->append(new DeleteClipAction(sequence, i));
						deleted = true;
					}
				}
			}
		}
		for (int j=0;j<items.size();j++) {
			Media* m = item_to_media(items.at(j));
			if (delete_clips_in_clipboard_with_media(ca, m)) deleted = true;
		}
		if (deleted) {
			undo_stack.push(ca);
			update_ui(true);
		} else {
			delete ca;
		}
	}
}

void Project::clear() {
	// clear effects cache
	panel_effect_controls->clear_effects(true);

	// delete sequences first because it's important to close all the clips before deleting the media
	QVector<Media*> sequences = list_all_project_sequences();
	for (int i=0;i<sequences.size();i++) {
		delete sequences.at(i)->to_sequence();
		sequences.at(i)->set_sequence(NULL);
	}

	// delete everything else
	project_model.clear();
}

void Project::new_project() {
	// clear existing project
	set_sequence(NULL);
	panel_footage_viewer->set_media(NULL);
	clear();
	mainWindow->setWindowModified(false);
}

void Project::load_project(bool autorecovery) {
	new_project();

	LoadDialog ld(this, autorecovery);
	ld.exec();
}

void Project::save_folder(QXmlStreamWriter& stream, int type, bool set_ids_only, const QModelIndex& parent) {
	bool root = (!parent.parent().isValid());
	for (int i=0;i<project_model.rowCount(parent);i++) {
		const QModelIndex& item = project_model.index(i, 0, parent);
		Media* m = project_model.getItem(item);

		if (type == m->get_type()) {
			if (m->get_type() == MEDIA_TYPE_FOLDER) {
				if (set_ids_only) {
					m->temp_id = folder_id; // saves a temporary ID for matching in the project file
					folder_id++;
				} else {
					// if we're saving folders, save the folder
					stream.writeStartElement("folder");
					stream.writeAttribute("name", m->get_name());
					stream.writeAttribute("id", QString::number(m->temp_id));
					if (!item.parent().isValid()) {
						stream.writeAttribute("parent", "0");
					} else {
						stream.writeAttribute("parent", QString::number(project_model.getItem(item.parent())->temp_id));
					}
					stream.writeEndElement();
				}
				// save_folder(stream, item, type, set_ids_only);
			} else {
				int folder = root ? 0 : project_model.getItem(parent)->temp_id;
				if (type == MEDIA_TYPE_FOOTAGE) {
					Footage* f = m->to_footage();
					f->save_id = media_id;
					stream.writeStartElement("footage");
					stream.writeAttribute("id", QString::number(media_id));
					stream.writeAttribute("folder", QString::number(folder));
					stream.writeAttribute("name", f->name);
					stream.writeAttribute("url", proj_dir.relativeFilePath(f->url));
					stream.writeAttribute("duration", QString::number(f->length));
					stream.writeAttribute("using_inout", QString::number(f->using_inout));
					stream.writeAttribute("in", QString::number(f->in));
					stream.writeAttribute("out", QString::number(f->out));
					stream.writeAttribute("speed", QString::number(f->speed));
					for (int j=0;j<f->video_tracks.size();j++) {
						const FootageStream& ms = f->video_tracks.at(j);
						stream.writeStartElement("video");
						stream.writeAttribute("id", QString::number(ms.file_index));
						stream.writeAttribute("width", QString::number(ms.video_width));
						stream.writeAttribute("height", QString::number(ms.video_height));
						stream.writeAttribute("framerate", QString::number(ms.video_frame_rate, 'f', 10));
						stream.writeAttribute("infinite", QString::number(ms.infinite_length));
						stream.writeEndElement();
					}
					for (int j=0;j<f->audio_tracks.size();j++) {
						const FootageStream& ms = f->audio_tracks.at(j);
						stream.writeStartElement("audio");
						stream.writeAttribute("id", QString::number(ms.file_index));
						stream.writeAttribute("channels", QString::number(ms.audio_channels));
						stream.writeAttribute("layout", QString::number(ms.audio_layout));
						stream.writeAttribute("frequency", QString::number(ms.audio_frequency));
						stream.writeEndElement();
					}
					stream.writeEndElement();
					media_id++;
				} else if (type == MEDIA_TYPE_SEQUENCE) {
					Sequence* s = m->to_sequence();
					if (set_ids_only) {
						s->save_id = sequence_id;
						sequence_id++;
					} else {
						stream.writeStartElement("sequence");
						stream.writeAttribute("id", QString::number(s->save_id));
						stream.writeAttribute("folder", QString::number(folder));
						stream.writeAttribute("name", s->name);
						stream.writeAttribute("width", QString::number(s->width));
						stream.writeAttribute("height", QString::number(s->height));
						stream.writeAttribute("framerate", QString::number(s->frame_rate, 'f', 10));
						stream.writeAttribute("afreq", QString::number(s->audio_frequency));
						stream.writeAttribute("alayout", QString::number(s->audio_layout));
						if (s == sequence) {
							stream.writeAttribute("open", "1");
						}
                        stream.writeAttribute("workarea", QString::number(s->using_workarea));
                        stream.writeAttribute("workareaEnabled", QString::number(s->enable_workarea));
						stream.writeAttribute("workareaIn", QString::number(s->workarea_in));
						stream.writeAttribute("workareaOut", QString::number(s->workarea_out));

						for (int j=0;j<s->transitions.size();j++) {
							Transition* t = s->transitions.at(j);
							if (t != NULL) {
								stream.writeStartElement("transition");
								stream.writeAttribute("id", QString::number(j));
								stream.writeAttribute("length", QString::number(t->get_true_length()));
								t->save(stream);
								stream.writeEndElement(); // transition
							}
						}

						for (int j=0;j<s->clips.size();j++) {
							Clip* c = s->clips.at(j);
							if (c != NULL) {
								stream.writeStartElement("clip"); // clip
								stream.writeAttribute("id", QString::number(j));
								stream.writeAttribute("enabled", QString::number(c->enabled));
								stream.writeAttribute("name", c->name);
								stream.writeAttribute("clipin", QString::number(c->clip_in));
								stream.writeAttribute("in", QString::number(c->timeline_in));
								stream.writeAttribute("out", QString::number(c->timeline_out));
								stream.writeAttribute("track", QString::number(c->track));
								stream.writeAttribute("opening", QString::number(c->opening_transition));
								stream.writeAttribute("closing", QString::number(c->closing_transition));

								stream.writeAttribute("r", QString::number(c->color_r));
								stream.writeAttribute("g", QString::number(c->color_g));
								stream.writeAttribute("b", QString::number(c->color_b));

								stream.writeAttribute("autoscale", QString::number(c->autoscale));
								stream.writeAttribute("speed", QString::number(c->speed, 'f', 10));
								stream.writeAttribute("maintainpitch", QString::number(c->maintain_audio_pitch));
								stream.writeAttribute("reverse", QString::number(c->reverse));

								if (c->media != NULL) {
									stream.writeAttribute("type", QString::number(c->media->get_type()));
									switch (c->media->get_type()) {
									case MEDIA_TYPE_FOOTAGE:
										stream.writeAttribute("media", QString::number(c->media->to_footage()->save_id));
										stream.writeAttribute("stream", QString::number(c->media_stream));
										break;
									case MEDIA_TYPE_SEQUENCE:
										stream.writeAttribute("sequence", QString::number(c->media->to_sequence()->save_id));
										break;
									}
								}

								stream.writeStartElement("linked"); // linked
								for (int k=0;k<c->linked.size();k++) {
									stream.writeStartElement("link"); // link
									stream.writeAttribute("id", QString::number(c->linked.at(k)));
									stream.writeEndElement(); // link
								}
								stream.writeEndElement(); // linked

								for (int k=0;k<c->effects.size();k++) {
									stream.writeStartElement("effect"); // effect
									c->effects.at(k)->save(stream);
									stream.writeEndElement(); // effect
								}

								stream.writeEndElement(); // clip
							}
						}
						for (int j=0;j<s->markers.size();j++) {
							stream.writeStartElement("marker");
							stream.writeAttribute("frame", QString::number(s->markers.at(j).frame));
							stream.writeAttribute("name", s->markers.at(j).name);
							stream.writeEndElement();
						}
						stream.writeEndElement();
					}
				}
			}
		}

		if (m->get_type() == MEDIA_TYPE_FOLDER) {
			save_folder(stream, type, set_ids_only, item);
		}
	}
}

void Project::save_project(bool autorecovery) {
	folder_id = 1;
	media_id = 1;
	sequence_id = 1;

	QFile file(autorecovery ? autorecovery_filename : project_url);
	if (!file.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
		dout << "[ERROR] Could not open file";
		return;
	}

	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument(); // doc

	stream.writeStartElement("project"); // project

	stream.writeTextElement("version", QString::number(SAVE_VERSION));

	stream.writeTextElement("url", project_url);
	proj_dir = QFileInfo(project_url).absoluteDir();

	save_folder(stream, MEDIA_TYPE_FOLDER, true);

	stream.writeStartElement("folders"); // folders
	save_folder(stream, MEDIA_TYPE_FOLDER, false);
	stream.writeEndElement(); // folders

	stream.writeStartElement("media"); // media
	save_folder(stream, MEDIA_TYPE_FOOTAGE, false);
	stream.writeEndElement(); // media

	save_folder(stream, MEDIA_TYPE_SEQUENCE, true);

	stream.writeStartElement("sequences"); // sequences
	save_folder(stream, MEDIA_TYPE_SEQUENCE, false);
	stream.writeEndElement();// sequences

	stream.writeEndElement(); // project

	stream.writeEndDocument(); // doc

	file.close();

	if (!autorecovery) {
		add_recent_project(project_url);
		mainWindow->setWindowModified(false);
	}
}

void Project::update_view_type() {
	tree_view->setVisible(config.project_view_type == PROJECT_VIEW_TREE);
	icon_view_container->setVisible(config.project_view_type == PROJECT_VIEW_ICON);

	switch (config.project_view_type) {
	case PROJECT_VIEW_TREE:
		sources_common->view = tree_view;
		break;
	case PROJECT_VIEW_ICON:
		sources_common->view = icon_view;
		break;
	}
}

void Project::set_icon_view() {
	config.project_view_type = PROJECT_VIEW_ICON;
	update_view_type();
}

void Project::set_tree_view() {
	config.project_view_type = PROJECT_VIEW_TREE;
	update_view_type();
}

void Project::save_recent_projects() {
	// save to file
	QFile f(recent_proj_file);
	if (f.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
		QTextStream out(&f);
		for (int i=0;i<recent_projects.size();i++) {
			if (i > 0) {
				out << "\n";
			}
			out << recent_projects.at(i);
		}
		f.close();
	} else {
		dout << "[WARNING] Could not save recent projects";
	}
}

void Project::clear_recent_projects() {
	recent_projects.clear();
	save_recent_projects();
}

void Project::set_icon_view_size(int s) {
	icon_view->setIconSize(QSize(s, s));
}

void Project::set_up_dir_enabled() {
	directory_up->setEnabled(icon_view->rootIndex().isValid());
}

void Project::go_up_dir() {
	icon_view->setRootIndex(icon_view->rootIndex().parent());
	set_up_dir_enabled();
}

void Project::make_new_menu() {
	QMenu new_menu(this);
	mainWindow->make_new_menu(&new_menu);
	new_menu.exec(QCursor::pos());
}

void Project::add_recent_project(QString url) {
	bool found = false;
	for (int i=0;i<recent_projects.size();i++) {
		if (url == recent_projects.at(i)) {
			found = true;
			recent_projects.move(i, 0);
			break;
		}
	}
	if (!found) {
		recent_projects.insert(0, url);
		if (recent_projects.size() > MAXIMUM_RECENT_PROJECTS) {
			recent_projects.removeLast();
		}
	}
	save_recent_projects();
}

void Project::list_all_sequences_worker(QVector<Media*>* list, Media* parent) {
	for (int i=0;i<project_model.childCount(parent);i++) {
		Media* item = project_model.child(i, parent);
		switch (item->get_type()) {
		case MEDIA_TYPE_SEQUENCE:
			list->append(item);
			break;
		case MEDIA_TYPE_FOLDER:
			list_all_sequences_worker(list, item);
			break;
		}
	}
}

QVector<Media*> Project::list_all_project_sequences() {
	QVector<Media*> list;
	list_all_sequences_worker(&list, NULL);
	return list;
}

QModelIndexList Project::get_current_selected() {
	if (config.project_view_type == PROJECT_VIEW_TREE) {
		return panel_project->tree_view->selectionModel()->selectedRows();
	}
	return panel_project->icon_view->selectionModel()->selectedIndexes();
}

#define THROBBER_LIMIT 20
#define THROBBER_SIZE 50

MediaThrobber::MediaThrobber(Media *i) : pixmap(":/icons/throbber.png"), animation(0), item(i), animator(NULL) {}

void MediaThrobber::start() {
	// set up throbber
	animation_update();
	animator = new QTimer(this);
	animator->setInterval(20);
	connect(animator, SIGNAL(timeout()), this, SLOT(animation_update()));
	animator->start();
}

void MediaThrobber::animation_update() {
	if (animation == THROBBER_LIMIT) {
		animation = 0;
	}
	project_model.set_icon(item, QIcon(pixmap.copy(THROBBER_SIZE*animation, 0, THROBBER_SIZE, THROBBER_SIZE)));
	animation++;
}

void MediaThrobber::stop(int icon_type, bool replace) {
	if (animator != NULL) {
		animator->stop();
		delete animator;
	}

	switch (icon_type) {
	case ICON_TYPE_VIDEO: project_model.set_icon(item, QIcon(":/icons/videosource.png")); break;
	case ICON_TYPE_AUDIO: project_model.set_icon(item, QIcon(":/icons/audiosource.png")); break;
	case ICON_TYPE_IMAGE: project_model.set_icon(item, QIcon(":/icons/imagesource.png")); break;
	case ICON_TYPE_ERROR: project_model.set_icon(item, QIcon(":/icons/error.png")); break;
	}

	// refresh all clips
	QVector<Media*> sequences = panel_project->list_all_project_sequences();
	for (int i=0;i<sequences.size();i++) {
		Sequence* s = sequences.at(i)->to_sequence();
		for (int j=0;j<s->clips.size();j++) {
			Clip* c = s->clips.at(j);
			if (c != NULL) {
				c->refresh();
			}
		}
	}

	// redraw clips
	update_ui(replace);

	panel_project->tree_view->viewport()->update();
	item->throbber = NULL;
	deleteLater();
}
