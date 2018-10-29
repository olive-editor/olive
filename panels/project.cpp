#include "project.h"
#include "ui_project.h"
#include "io/media.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "playback/playback.h"
#include "effects/effect.h"
#include "panels/timeline.h"
#include "project/sequence.h"
#include "effects/effect.h"
#include "effects/transition.h"
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
#include "debug.h"

#include <QFileDialog>
#include <QString>
#include <QVariant>
#include <QCharRef>
#include <QMessageBox>
#include <QDropEvent>
#include <QMimeData>
#include <QPushButton>
#include <QInputDialog>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}

#define MAXIMUM_RECENT_PROJECTS 10

QString autorecovery_filename;
QString project_url = "";
QStringList recent_projects;
QString recent_proj_file;

Project::Project(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Project)
{
    ui->setupUi(this);
    source_table = ui->treeWidget;
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(rename_media(QTreeWidgetItem*,int)));
}

Project::~Project()
{
	delete ui;
}

QString Project::get_next_sequence_name() {
	int n = 1;
	bool found = true;
	QString name;
	while (found) {
		found = false;
		name = "Sequence ";
		if (n < 10) {
			name += "0";
		}
		name += QString::number(n);
		for (int i=0;i<ui->treeWidget->topLevelItemCount();i++) {
			if (QString::compare(ui->treeWidget->topLevelItem(i)->text(0), name, Qt::CaseInsensitive) == 0) {
				found = true;
				n++;
				break;
			}
		}
	}
	return name;
}

void Project::rename_media(QTreeWidgetItem* item, int column) {
    int type = get_type_from_tree(item);
    QString n = item->text(column);
    switch (type) {
	case MEDIA_TYPE_FOOTAGE: get_footage_from_tree(item)->name = n; break;
    case MEDIA_TYPE_SEQUENCE: get_sequence_from_tree(item)->name = n; break;
    }
}

void Project::duplicate_selected() {
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    bool duped = false;
    ComboAction* ca = new ComboAction();
    for (int j=0;j<items.size();j++) {
        QTreeWidgetItem* i = items.at(j);
        if (get_type_from_tree(i) == MEDIA_TYPE_SEQUENCE) {
            new_sequence(ca, get_sequence_from_tree(i)->copy(), false, i->parent());
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
	QList<QTreeWidgetItem*> selected_items = ui->treeWidget->selectedItems();
	if (selected_items.size() == 1) {
		QTreeWidgetItem* item = selected_items.at(0);
		if (get_type_from_tree(item) == MEDIA_TYPE_FOOTAGE) {
			replace_media(item, 0);
		}
	}
}

void Project::replace_media(QTreeWidgetItem* item, QString filename) {
	if (filename.isEmpty()) {
		filename = QFileDialog::getOpenFileName(this, "Replace '" + item->text(0) + "'", "", "All Files (*)");
	}
	if (!filename.isEmpty()) {
		ReplaceMediaCommand* rmc = new ReplaceMediaCommand(item, filename);
		undo_stack.push(rmc);
	}
}

void Project::replace_clip_media() {
	if (sequence == NULL) {
		QMessageBox::critical(this, "No active sequence", "No sequence is active, please open the sequence you want to replace clips from.", QMessageBox::Ok);
	} else if (ui->treeWidget->selectedItems().size() == 1) {
		QTreeWidgetItem* item = ui->treeWidget->selectedItems().at(0);
		if (get_type_from_tree(item) == MEDIA_TYPE_SEQUENCE && sequence == get_sequence_from_tree(item)) {
			QMessageBox::critical(this, "Active sequence selected", "You cannot insert a sequence into itself, so no clips of this media would be in this sequence.", QMessageBox::Ok);
		} else {
			ReplaceClipMediaDialog dialog(this, ui->treeWidget, item);
			dialog.exec();
		}
	}
}

void Project::open_properties() {
	if (ui->treeWidget->selectedItems().size() == 1) {
		QTreeWidgetItem* item = ui->treeWidget->selectedItems().at(0);
		switch (get_type_from_tree(item)) {
		case MEDIA_TYPE_FOOTAGE:
		{
            MediaPropertiesDialog mpd(this, item, get_footage_from_tree(item));
			mpd.exec();
		}
			break;
		case MEDIA_TYPE_SEQUENCE:
		{
			NewSequenceDialog nsd(this);
			Sequence* s = get_sequence_from_tree(item);
			nsd.existing_sequence = s;
			nsd.existing_item = item;
			nsd.exec();
		}
			break;
		default:
		{
			// fall back to renaming
			QString new_name = QInputDialog::getText(this, "Rename '" + item->text(0) + "'", "Enter new name:", QLineEdit::Normal, item->text(0));
			if (!new_name.isEmpty()) {
				MediaRename* mr = new MediaRename();
				mr->from = item->text(0);
				mr->item = item;
				mr->to = new_name;
				undo_stack.push(mr);
			}
		}
		}
	}
}

void Project::new_sequence(ComboAction *ca, Sequence *s, bool open, QTreeWidgetItem* parent) {
    QTreeWidgetItem* item = new_item();
    item->setText(0, s->name);
    set_sequence_of_tree(item, s);

    if (ca != NULL) {
        ca->append(new NewSequenceCommand(item, parent));
        if (open) ca->append(new ChangeSequenceAction(s));
    } else {
        if (parent == NULL) {
            ui->treeWidget->addTopLevelItem(item);
        } else {
            parent->addChild(item);
        }
        if (open) set_sequence(s);
	}
}

void Project::start_preview_generator(QTreeWidgetItem* item, Media* media, bool replacing) {
    // set up throbber animation
    MediaThrobber* throbber = new MediaThrobber(item);

	PreviewGenerator* pg = new PreviewGenerator(item, media, replacing);
	media->preview_gen = pg;
	connect(pg, SIGNAL(set_icon(int, bool)), throbber, SLOT(stop(int, bool)));
    pg->start(QThread::LowPriority);
}

QString Project::get_file_name_from_path(const QString& path) {
	return path.mid(path.lastIndexOf('/')+1);
}

QTreeWidgetItem* Project::new_item() {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    return item;
}

bool Project::is_focused() {
    return ui->treeWidget->hasFocus();
}

QTreeWidgetItem* Project::new_folder(QString name) {
    QTreeWidgetItem* item = new_item();
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	item->setText(0, (name.isEmpty()) ? "New Folder" : name);
	item->setIcon(0, QIcon(":/icons/folder.png"));
    set_item_to_folder(item);
    return item;
}

void Project::get_all_media_from_table(QList<QTreeWidgetItem*> items, QList<QTreeWidgetItem*>& list, int search_type) {
    for (int i=0;i<items.size();i++) {
        QTreeWidgetItem* item = items.at(i);
        int type = get_type_from_tree(item);
        if (type == MEDIA_TYPE_FOLDER) {
            QList<QTreeWidgetItem*> children;
            for (int j=0;j<item->childCount();j++) {
                children.append(item->child(j));
            }
			get_all_media_from_table(children, list, search_type);
		} else if (search_type == type || search_type == -1) {
            list.append(item);
        }
    }
}

void Project::delete_selected_media() {
    ComboAction* ca = new ComboAction();
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    bool remove = true;
    bool redraw = false;

    // check if media is in use
    QVector<QTreeWidgetItem*> parents;
    QList<QTreeWidgetItem*> sequence_items;
    QList<QTreeWidgetItem*> all_top_level_items;
    for (int i=0;i<ui->treeWidget->topLevelItemCount();i++) {
        all_top_level_items.append(ui->treeWidget->topLevelItem(i));
    }
	get_all_media_from_table(all_top_level_items, sequence_items, MEDIA_TYPE_SEQUENCE); // find all sequences in project
    if (sequence_items.size() > 0) {
        QList<QTreeWidgetItem*> media_items;
		get_all_media_from_table(items, media_items, MEDIA_TYPE_FOOTAGE);
        for (int i=0;i<media_items.size();i++) {
            QTreeWidgetItem* item = media_items.at(i);
			Media* media = get_footage_from_tree(item);
            bool confirm_delete = false;
            for (int j=0;j<sequence_items.size();j++) {
                Sequence* s = get_sequence_from_tree(sequence_items.at(j));
                for (int k=0;k<s->clips.size();k++) {
                    Clip* c = s->clips.at(k);
                    if (c != NULL && c->media == media) {
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
                                QTreeWidgetItem* parent = item;
                                while (parent != NULL) {
                                    parents.append(parent);

                                    // re-add item's siblings
                                    for (int m=0;m<parent->childCount();m++) {
                                        QTreeWidgetItem* child = parent->child(m);
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

                                    parent = parent->parent();
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
        }
    }

    // remove
    if (remove) {
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

            if (get_type_from_tree(items.at(i)) == MEDIA_TYPE_SEQUENCE) {
                redraw = true;

                if (get_sequence_from_tree(items.at(i)) == sequence) {
                    ca->append(new ChangeSequenceAction(NULL));
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

void Project::process_file_list(bool recursive, QStringList& files, QTreeWidgetItem* parent, QTreeWidgetItem* replace) {
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
			QTreeWidgetItem* folder = new_folder(folder_name);

			QDir directory(files.at(i));
			directory.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

			QFileInfoList subdir_files = directory.entryInfoList();
			QStringList subdir_filenames;

			for (int j=0;j<subdir_files.size();j++) {
				subdir_filenames.append(subdir_files.at(j).filePath());
			}

			process_file_list(true, subdir_filenames, folder, NULL);

			if (create_undo_action) {
                ca->append(new AddMediaCommand(folder, parent));
			} else {
				parent->addChild(folder);
			}

			imported = true;
		} else {
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
				QTreeWidgetItem* item;
				Media* m;

				if (replace != NULL) {
					item = replace;
					m = get_footage_from_tree(replace);
					m->reset();
				} else {
					item = new_item();
					m = new Media();
				}

                m->using_inout = false;
				m->url = file;
				m->name = get_file_name_from_path(files.at(i));

				// generate waveform/thumbnail in another thread
				start_preview_generator(item, m, replace != NULL);

				set_footage_of_tree(item, m);

				last_imported_media.append(m);

				if (replace == NULL) {
					if (create_undo_action) {
                        ca->append(new AddMediaCommand(item, parent));
					} else {
						parent->addChild(item);
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

QTreeWidgetItem* Project::get_selected_folder() {
	// if one item is selected and it's a folder, return it
	QList<QTreeWidgetItem*> selected_items = panel_project->source_table->selectedItems();
	if (selected_items.size() == 1 && get_type_from_tree(selected_items.at(0)) == MEDIA_TYPE_FOLDER) {
		return selected_items.at(0);
	}
	return NULL;
}

bool Project::reveal_media(void *media, QTreeWidgetItem* parent) {
	int count = (parent == NULL) ? ui->treeWidget->topLevelItemCount() : parent->childCount();

	for (int i=0;i<count;i++) {
		QTreeWidgetItem* item = (parent == NULL) ? ui->treeWidget->topLevelItem(i) : parent->child(i);
		if (get_type_from_tree(item) == MEDIA_TYPE_FOLDER) {
			if (reveal_media(media, item)) return true;
		} else if (get_media_from_tree(item) == media) {
			// expand all folders leading to this media
			QTreeWidgetItem* hierarchy = item->parent();
			while (hierarchy != NULL) {
				hierarchy->setExpanded(true);
				hierarchy = hierarchy->parent();
			}

			// select item
			item->setSelected(true);

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
		process_file_list(false, files, get_selected_folder(), NULL);
	}
}

void set_item_to_folder(QTreeWidgetItem* item) {
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_FOLDER);
}

void* get_media_from_tree(QTreeWidgetItem* item) {
	return reinterpret_cast<void*>(item->data(0, Qt::UserRole + 2).value<quintptr>());
	/*int type = get_type_from_tree(item);
	switch (type) {
	case MEDIA_TYPE_FOOTAGE: return get_footage_from_tree(item);
	case MEDIA_TYPE_SEQUENCE: return get_sequence_from_tree(item);
	default: dout << "[ERROR] Invalid media type when retrieving media";
	}
	return NULL;*/
}

Media* get_footage_from_tree(QTreeWidgetItem* item) {
    return reinterpret_cast<Media*>(item->data(0, Qt::UserRole + 2).value<quintptr>());
}

void set_footage_of_tree(QTreeWidgetItem* item, Media* media) {
    item->setText(0, media->name);
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_FOOTAGE);
    item->setData(0, Qt::UserRole + 2, QVariant::fromValue(reinterpret_cast<quintptr>(media)));
}

Sequence* get_sequence_from_tree(QTreeWidgetItem* item) {
    return reinterpret_cast<Sequence*>(item->data(0, Qt::UserRole + 2).value<quintptr>());
}

QString get_channel_layout_name(int channels, int layout) {
	switch (channels) {
	case 0: return "Invalid"; break;
	case 1: return "Mono"; break;
	case 2: return "Stereo"; break;
	default: {
		char buf[50];
		av_get_channel_layout_string(buf, sizeof(buf), channels, layout);
		return QString(buf);
	}
	}
}

void set_sequence_of_tree(QTreeWidgetItem* item, Sequence* s) {
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_SEQUENCE);
	item->setData(0, Qt::UserRole + 2, QVariant::fromValue(reinterpret_cast<quintptr>(s)));
	item->setToolTip(0, "Name: " + s->name
					 + "\nVideo Dimensions: " + QString::number(s->width) + "x" + QString::number(s->height)
					 + "\nFrame Rate: " + QString::number(s->frame_rate)
					 + "\nAudio Frequency: " + QString::number(s->audio_frequency)
					 + "\nAudio Layout: " + get_channel_layout_name(av_get_channel_layout_nb_channels(s->audio_layout), s->audio_layout));
	item->setIcon(0, QIcon(":/icons/sequence.png"));
}

int get_type_from_tree(QTreeWidgetItem* item) {
    return item->data(0, Qt::UserRole + 1).toInt();
}

void Project::delete_media(QTreeWidgetItem* item) {
    int type = get_type_from_tree(item);
    void* media = get_media_from_tree(item);
    switch (type) {
    case MEDIA_TYPE_FOOTAGE:
        delete static_cast<Media*>(media);
        break;
    case MEDIA_TYPE_SEQUENCE:
        delete static_cast<Sequence*>(media);
        break;
    }
}

void Project::delete_clips_using_selected_media() {
	if (sequence == NULL) {
		QMessageBox::critical(this, "No active sequence", "No sequence is active, please open the sequence you want to delete clips from.", QMessageBox::Ok);
	} else {
        ComboAction* ca = new ComboAction();
		bool deleted = false;
        for (int i=0;i<sequence->clips.size();i++) {
            Clip* c = sequence->clips.at(i);
			if (c != NULL) {
				QList<QTreeWidgetItem*> items = source_table->selectedItems();
				for (int j=0;j<items.size();j++) {
					Media* m = get_footage_from_tree(items.at(j));
					if (c->media == m) {
                        ca->append(new DeleteClipAction(sequence, i));
						deleted = true;
					}
				}
			}
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
    QVector<Sequence*> sequences = list_all_project_sequences();
    for (int i=0;i<sequences.size();i++) {
        delete sequences.at(i);
    }

    // delete everything else
    while (ui->treeWidget->topLevelItemCount() > 0) {
        QTreeWidgetItem* item = ui->treeWidget->topLevelItem(0);
        if (get_type_from_tree(item) != MEDIA_TYPE_SEQUENCE) delete_media(item); // already deleted
        delete item;
    }
}

void Project::new_project() {
	// clear existing project
    set_sequence(NULL);
    clear();
	mainWindow->setWindowModified(false);
}

QTreeWidgetItem* Project::find_loaded_folder_by_id(int id) {
    for (int j=0;j<loaded_folders.size();j++) {
        QTreeWidgetItem* parent_item = loaded_folders.at(j);
        if (parent_item->data(0, Qt::UserRole + 3).toInt() == id) {
            return parent_item;
        }
    }
    return NULL;
}

bool Project::load_worker(QFile& f, QXmlStreamReader& stream, int type) {
    f.seek(0);
    stream.setDevice(stream.device());

    QString root_search;
    QString child_search;

    switch (type) {
    case LOAD_TYPE_VERSION:
        root_search = "version";
        break;
    case MEDIA_TYPE_FOLDER:
        root_search = "folders";
        child_search = "folder";
        break;
    case MEDIA_TYPE_FOOTAGE:
        root_search = "media";
        child_search = "footage";
        break;
    case MEDIA_TYPE_SEQUENCE:
        root_search = "sequences";
        child_search = "sequence";
        break;
    }

    show_err = true;

    while (!stream.atEnd()) {
        stream.readNextStartElement();
        if (stream.name() == root_search) {
            if (type == LOAD_TYPE_VERSION) {
                if (stream.readElementText() != SAVE_VERSION) {
                    if (QMessageBox::warning(this, "Version Mismatch", "This project was saved in a different version of Olive and may not be fully compatible with this version. Would you like to attempt loading it anyway?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                        show_err = false;
                        return false;
                    }
                }
            } else {
                while (!(stream.name() == root_search && stream.isEndElement())) {
                    stream.readNext();
                    if (stream.name() == child_search && stream.isStartElement()) {
                        switch (type) {
                        case MEDIA_TYPE_FOLDER:
                        {
							QTreeWidgetItem* folder = new_folder(0);
                            for (int j=0;j<stream.attributes().size();j++) {
                                const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                if (attr.name() == "id") {
                                    folder->setData(0, Qt::UserRole + 3, attr.value().toInt());
                                } else if (attr.name() == "name") {
                                    folder->setText(0, attr.value().toString());
                                } else if (attr.name() == "parent") {
                                    folder->setData(0, Qt::UserRole + 4, attr.value().toInt());
                                }
                            }
                            loaded_folders.append(folder);
                        }
                            break;
                        case MEDIA_TYPE_FOOTAGE:
                        {
                            int folder = 0;

							QTreeWidgetItem* item = new_item();
                            Media* m = new Media();

                            // TODO make save/load-able
                            m->using_inout = false;

                            for (int j=0;j<stream.attributes().size();j++) {
                                const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                if (attr.name() == "id") {
                                    m->save_id = attr.value().toInt();
                                } else if (attr.name() == "folder") {
                                    folder = attr.value().toInt();
                                } else if (attr.name() == "name") {
                                    m->name = attr.value().toString();
                                } else if (attr.name() == "url") {
                                    m->url = attr.value().toString();
                                } else if (attr.name() == "duration") {
                                    m->length = attr.value().toLongLong();
                                }
							}

							set_footage_of_tree(item, m);

                            if (folder == 0) {
                                ui->treeWidget->addTopLevelItem(item);
                            } else {
                                find_loaded_folder_by_id(folder)->addChild(item);
                            }

                            // analyze media to see if it's the same
							start_preview_generator(item, m, true);

                            loaded_media.append(m);
                        }
                            break;
                        case MEDIA_TYPE_SEQUENCE:
                        {
                            QTreeWidgetItem* parent = NULL;
                            Sequence* s = new Sequence();

                            // load attributes about sequence
                            for (int j=0;j<stream.attributes().size();j++) {
                                const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                if (attr.name() == "name") {
                                    s->name = attr.value().toString();
                                } else if (attr.name() == "folder") {
                                    int folder = attr.value().toInt();
                                    if (folder > 0) parent = find_loaded_folder_by_id(folder);
                                } else if (attr.name() == "id") {
                                    s->save_id = attr.value().toInt();
                                } else if (attr.name() == "width") {
                                    s->width = attr.value().toInt();
                                } else if (attr.name() == "height") {
                                    s->height = attr.value().toInt();
                                } else if (attr.name() == "framerate") {
                                    s->frame_rate = attr.value().toFloat();
                                } else if (attr.name() == "afreq") {
                                    s->audio_frequency = attr.value().toInt();
                                } else if (attr.name() == "alayout") {
                                    s->audio_layout = attr.value().toInt();
                                } else if (attr.name() == "open") {
                                    open_seq = s;
								} else if (attr.name() == "workarea") {
									s->using_workarea = (attr.value() == "1");
                                } else if (attr.name() == "workareaIn") {                                    
                                    s->workarea_in = attr.value().toLong();
                                } else if (attr.name() == "workareaOut") {
                                    s->workarea_out = attr.value().toLong();
                                }
                            }

                            // load all clips and clip information
                            while (!(stream.name() == child_search && stream.isEndElement()) && !stream.atEnd()) {
                                stream.readNextStartElement();
								if (stream.name() == "marker" && stream.isStartElement()) {
									Marker m;
									for (int j=0;j<stream.attributes().size();j++) {
										const QXmlStreamAttribute& attr = stream.attributes().at(j);
										if (attr.name() == "frame") {
											m.frame = attr.value().toLong();
										} else if (attr.name() == "name") {
											m.name = attr.value().toString();
										}
									}
									s->markers.append(m);
								} else if (stream.name() == "clip" && stream.isStartElement()) {
                                    int media_id, stream_id;
                                    Clip* c = new Clip(s);

                                    // backwards compatibility code
									c->autoscale = false;

                                    c->media = NULL;
                                    for (int j=0;j<stream.attributes().size();j++) {
                                        const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                        if (attr.name() == "name") {
                                            c->name = attr.value().toString();
										} else if (attr.name() == "enabled") {
											c->enabled = (attr.value() == "1");
                                        } else if (attr.name() == "id") {
                                            c->load_id = attr.value().toInt();
                                        } else if (attr.name() == "clipin") {
                                            c->clip_in = attr.value().toLong();
                                        } else if (attr.name() == "in") {
                                            c->timeline_in = attr.value().toLong();
                                        } else if (attr.name() == "out") {
                                            c->timeline_out = attr.value().toLong();
                                        } else if (attr.name() == "track") {
                                            c->track = attr.value().toInt();
                                        } else if (attr.name() == "r") {
                                            c->color_r = attr.value().toInt();
                                        } else if (attr.name() == "g") {
                                            c->color_g = attr.value().toInt();
                                        } else if (attr.name() == "b") {
                                            c->color_b = attr.value().toInt();
                                        } else if (attr.name() == "autoscale") {
                                            c->autoscale = (attr.value() == "1");
										} else if (attr.name() == "type") {
											c->media_type = attr.value().toInt();
                                        } else if (attr.name() == "media") {
                                            c->media_type = MEDIA_TYPE_FOOTAGE;
                                            media_id = attr.value().toInt();
                                        } else if (attr.name() == "stream") {
                                            stream_id = attr.value().toInt();
										} else if (attr.name() == "speed") {
											c->speed = attr.value().toDouble();
                                        } else if (attr.name() == "maintainpitch") {
                                            c->maintain_audio_pitch = (attr.value() == "1");
                                        } else if (attr.name() == "reverse") {
                                            c->reverse = (attr.value() == "1");
                                        } else if (attr.name() == "sequence") {
                                            c->media_type = MEDIA_TYPE_SEQUENCE;

                                            // since we haven't finished loading sequences, we defer linking this until later
                                            c->media = NULL;
                                            c->media_stream = attr.value().toInt();
                                            loaded_clips.append(c);
                                        }
                                    }

                                    // set media and media stream
									switch (c->media_type) {
                                    case MEDIA_TYPE_FOOTAGE:
										if (media_id == 0) {
											c->media = NULL;
										} else {
											for (int j=0;j<loaded_media.size();j++) {
												Media* m = loaded_media.at(j);
												if (m->save_id == media_id) {
													c->media = m;
													c->media_stream = stream_id;
													break;
												}
											}
										}
										break;
									}

                                    // load links and effects
                                    while (!(stream.name() == "clip" && stream.isEndElement()) && !stream.atEnd()) {
                                        stream.readNext();
                                        if (stream.isStartElement()) {
                                            if (stream.name() == "linked") {
                                                while (!(stream.name() == "linked" && stream.isEndElement()) && !stream.atEnd()) {
                                                    stream.readNext();
                                                    if (stream.name() == "link" && stream.isStartElement()) {
                                                        for (int k=0;k<stream.attributes().size();k++) {
                                                            const QXmlStreamAttribute& link_attr = stream.attributes().at(k);
                                                            if (link_attr.name() == "id") {
                                                                c->linked.append(link_attr.value().toInt());
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else if (stream.name() == "effects") {
                                                while (!(stream.name() == "effects" && stream.isEndElement()) && !stream.atEnd()) {
                                                    stream.readNext();
                                                    if (stream.name() == "effect" && stream.isStartElement()) {
                                                        int effect_id = -1;
														bool effect_enabled = true;
                                                        for (int j=0;j<stream.attributes().size();j++) {
                                                            const QXmlStreamAttribute& attr = stream.attributes().at(j);
															if (attr.name() == "id") {
																effect_id = attr.value().toInt();
															} else if (attr.name() == "enabled") {
																effect_enabled = (attr.value() == "1");
															}
                                                        }
                                                        if (effect_id != -1) {
                                                            Effect* e = create_effect(effect_id, c);
															e->set_enabled(effect_enabled);
//															stream.writeAttribute("enabled", QString::number(e->is_enabled()));
                                                            e->load(stream);
                                                            c->effects.append(e);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    s->clips.append(c);
                                }
                            }

                            // correct links and clip IDs
                            for (int i=0;i<s->clips.size();i++) {
                                // correct links
                                Clip* correct_clip = s->clips.at(i);
                                for (int j=0;j<correct_clip->linked.size();j++) {
                                    bool found = false;
                                    for (int k=0;k<s->clips.size();k++) {
                                        if (s->clips.at(k)->load_id == correct_clip->linked.at(j)) {
                                            correct_clip->linked[j] = k;
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) {
                                        correct_clip->linked.removeAt(j);
                                        j--;
                                        if (QMessageBox::warning(this, "Invalid Clip Link", "This project contains an invalid clip link. It may be corrupt. Would you like to continue loading it?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                                            delete s;
                                            return false;
                                        }
                                    }
                                }
                            }

                            new_sequence(NULL, s, false, parent);

                            loaded_sequences.append(s);
                        }
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
    return true;
}

void Project::load_project() {
    new_project();

	/*LoadDialog ld;
	ld.exec();*/

    QFile file(project_url);
    if (!file.open(QIODevice::ReadOnly)) {
		dout << "[ERROR] Could not open file";
        return;
    }

    QXmlStreamReader stream(&file);

    bool cont = false;
    error_str.clear();
    show_err = true;

    // temp variables for loading
    loaded_folders.clear();
    loaded_media.clear();
    loaded_clips.clear();
    loaded_sequences.clear();
    open_seq = NULL;

    // find project file version
    cont = load_worker(file, stream, LOAD_TYPE_VERSION);

    // load folders first
    if (cont) {
        cont = load_worker(file, stream, MEDIA_TYPE_FOLDER);
    }

    // load media
    if (cont) {
        // since folders loaded correctly, organize them appropriately
        for (int i=0;i<loaded_folders.size();i++) {
            QTreeWidgetItem* folder = loaded_folders.at(i);
            int parent = folder->data(0, Qt::UserRole + 4).toInt();
			if (parent > 0) {
                find_loaded_folder_by_id(parent)->addChild(folder);
			} else {
				ui->treeWidget->addTopLevelItem(folder);
			}
        }

        cont = load_worker(file, stream, MEDIA_TYPE_FOOTAGE);
    }

    // load sequences
    if (cont) {
        cont = load_worker(file, stream, MEDIA_TYPE_SEQUENCE);
    }

    // attach nested sequence clips to their sequences
    for (int i=0;i<loaded_clips.size();i++) {
        for (int j=0;j<loaded_sequences.size();j++) {
            if (loaded_clips.at(i)->media_stream == loaded_sequences.at(j)->save_id) {
                loaded_clips.at(i)->media = loaded_sequences.at(j);
                loaded_clips.at(i)->refresh();
                break;
            }
        }
    }

    if (!cont) {
        if (show_err) QMessageBox::critical(this, "Project Load Error", "Error loading project: " + error_str, QMessageBox::Ok);
    } else if (stream.hasError()) {
		dout << "[ERROR] Error parsing XML." << stream.errorString();
        QMessageBox::critical(this, "XML Parsing Error", "Couldn't load '" + project_url + "'. " + stream.errorString(), QMessageBox::Ok);
        cont = false;
    }

    if (cont) {
        if (open_seq != NULL) set_sequence(open_seq);

		update_ui(false);
		mainWindow->setWindowModified(false);
    } else {
        new_project();
    }

    add_recent_project(project_url);

    file.close();
}

void Project::save_folder(QXmlStreamWriter& stream, QTreeWidgetItem* parent, int type, bool set_ids_only) {
    bool root = (parent == NULL);
    int len = root ? ui->treeWidget->topLevelItemCount() : parent->childCount();
    for (int i=0;i<len;i++) {
        QTreeWidgetItem* item = root ? ui->treeWidget->topLevelItem(i) : parent->child(i);
		int item_type = get_type_from_tree(item);

        if (type == item_type) {
            if (item_type == MEDIA_TYPE_FOLDER) {
                if (set_ids_only) {
                    item->setData(0, Qt::UserRole + 3, folder_id); // saves a temporary ID for matching in the project file
                    folder_id++;
                } else {
                    // if we're saving folders, save the folder
                    stream.writeStartElement("folder");
                    stream.writeAttribute("name", item->text(0));
                    stream.writeAttribute("id", QString::number(item->data(0, Qt::UserRole + 3).toInt()));
                    if (item->parent() == NULL) {
                        stream.writeAttribute("parent", "0");
                    } else {
                        stream.writeAttribute("parent", QString::number(item->parent()->data(0, Qt::UserRole + 3).toInt()));
                    }
                    stream.writeEndElement();
                }
				// save_folder(stream, item, type, set_ids_only);
            } else {
                int folder = root ? 0 : parent->data(0, Qt::UserRole + 3).toInt();
                if (type == MEDIA_TYPE_FOOTAGE) {
					Media* m = get_footage_from_tree(item);
                    m->save_id = media_id;
                    stream.writeStartElement("footage");
                    stream.writeAttribute("id", QString::number(media_id));
                    stream.writeAttribute("folder", QString::number(folder));
                    stream.writeAttribute("name", m->name);
                    stream.writeAttribute("url", m->url);
                    stream.writeAttribute("duration", QString::number(m->length));
                    for (int j=0;j<m->video_tracks.size();j++) {
                        MediaStream* ms = m->video_tracks.at(j);
                        stream.writeStartElement("video");
                        stream.writeAttribute("id", QString::number(ms->file_index));
                        stream.writeAttribute("width", QString::number(ms->video_width));
                        stream.writeAttribute("height", QString::number(ms->video_height));
						stream.writeAttribute("framerate", QString::number(ms->video_frame_rate, 'f', 10));
                        stream.writeAttribute("infinite", QString::number(ms->infinite_length));
                        stream.writeEndElement();
                    }
                    for (int j=0;j<m->audio_tracks.size();j++) {
                        MediaStream* ms = m->audio_tracks.at(j);
                        stream.writeStartElement("audio");
                        stream.writeAttribute("id", QString::number(ms->file_index));
                        stream.writeAttribute("channels", QString::number(ms->audio_channels));
                        stream.writeAttribute("layout", QString::number(ms->audio_layout));
                        stream.writeAttribute("frequency", QString::number(ms->audio_frequency));
                        stream.writeEndElement();
                    }
                    stream.writeEndElement();
                    media_id++;
                } else if (type == MEDIA_TYPE_SEQUENCE) {
                    Sequence* s = get_sequence_from_tree(item);
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
						stream.writeAttribute("workareaIn", QString::number(s->workarea_in));
						stream.writeAttribute("workareaOut", QString::number(s->workarea_out));
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
                                if (c->opening_transition != NULL) {
                                    stream.writeAttribute("opening", QString::number(c->opening_transition->id));
                                }
                                if (c->closing_transition != NULL) {
                                    stream.writeAttribute("closing", QString::number(c->closing_transition->id));
                                }
                                stream.writeAttribute("r", QString::number(c->color_r));
                                stream.writeAttribute("g", QString::number(c->color_g));
                                stream.writeAttribute("b", QString::number(c->color_b));

                                stream.writeAttribute("autoscale", QString::number(c->autoscale));
								stream.writeAttribute("speed", QString::number(c->speed, 'f', 10));
                                stream.writeAttribute("maintainpitch", QString::number(c->maintain_audio_pitch));
                                stream.writeAttribute("reverse", QString::number(c->reverse));

                                stream.writeAttribute("type", QString::number(c->media_type));
                                switch (c->media_type) {
                                case MEDIA_TYPE_FOOTAGE:
                                    stream.writeAttribute("media", QString::number(static_cast<Media*>(c->media)->save_id));
                                    stream.writeAttribute("stream", QString::number(c->media_stream));
                                    break;
                                case MEDIA_TYPE_SEQUENCE:
                                    stream.writeAttribute("sequence", QString::number(static_cast<Sequence*>(c->media)->save_id));
                                    break;
                                }

                                stream.writeStartElement("linked"); // linked
                                for (int k=0;k<c->linked.size();k++) {
                                    stream.writeStartElement("link"); // link
                                    stream.writeAttribute("id", QString::number(c->linked.at(k)));
                                    stream.writeEndElement(); // link
                                }
                                stream.writeEndElement(); // linked

                                stream.writeStartElement("effects"); // effects
                                for (int k=0;k<c->effects.size();k++) {
                                    stream.writeStartElement("effect"); // effect
                                    Effect* e = c->effects.at(k);
                                    stream.writeAttribute("id", QString::number(e->id));									
									stream.writeAttribute("enabled", QString::number(e->is_enabled()));
                                    e->save(stream);
                                    stream.writeEndElement(); // effect
                                }
                                stream.writeEndElement(); // effects
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

		if (item_type == MEDIA_TYPE_FOLDER) {
			save_folder(stream, item, type, set_ids_only);
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

    stream.writeTextElement("version", SAVE_VERSION);

    save_folder(stream, NULL, MEDIA_TYPE_FOLDER, true);

    stream.writeStartElement("folders"); // folders
    save_folder(stream, NULL, MEDIA_TYPE_FOLDER, false);
    stream.writeEndElement(); // folders

    stream.writeStartElement("media"); // media
    save_folder(stream, NULL, MEDIA_TYPE_FOOTAGE, false);
    stream.writeEndElement(); // media

    save_folder(stream, NULL, MEDIA_TYPE_SEQUENCE, true);

    stream.writeStartElement("sequences"); // sequences
    save_folder(stream, NULL, MEDIA_TYPE_SEQUENCE, false);
    stream.writeEndElement();// sequences

    stream.writeEndElement(); // project

    stream.writeEndDocument(); // doc

    file.close();

    if (!autorecovery) {
        add_recent_project(project_url);
		mainWindow->setWindowModified(false);
    }
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

void Project::list_all_sequences_worker(QVector<Sequence*>* list, QTreeWidgetItem* parent) {
    int len = (parent == NULL) ? ui->treeWidget->topLevelItemCount() : parent->childCount();
    for (int i=0;i<len;i++) {
        QTreeWidgetItem* item = (parent == NULL) ? ui->treeWidget->topLevelItem(i) : parent->child(i);
        if (get_type_from_tree(item) == MEDIA_TYPE_SEQUENCE) {
            list->append(get_sequence_from_tree(item));
        } else if (get_type_from_tree(item) == MEDIA_TYPE_FOLDER) {
            list_all_sequences_worker(list, item);
        }
    }
}

QVector<Sequence*> Project::list_all_project_sequences() {
    QVector<Sequence*> list;
    list_all_sequences_worker(&list, NULL);
    return list;
}

#define THROBBER_LIMIT 20
#define THROBBER_SIZE 50

MediaThrobber::MediaThrobber(QTreeWidgetItem* i) : pixmap(":/icons/throbber.png"), animation(0), item(i) {
    // set up throbber
    animation_update();
    animator.setInterval(20);
    connect(&animator, SIGNAL(timeout()), this, SLOT(animation_update()));
    animator.start();
}

void MediaThrobber::animation_update() {
    if (animation == THROBBER_LIMIT) {
        animation = 0;
    }
    item->setIcon(0, QIcon(pixmap.copy(THROBBER_SIZE*animation, 0, THROBBER_SIZE, THROBBER_SIZE)));
    animation++;
}

void MediaThrobber::stop(int icon_type, bool replace) {
    animator.stop();

    switch (icon_type) {
    case ICON_TYPE_VIDEO: item->setIcon(0, QIcon(":/icons/videosource.png")); break;
    case ICON_TYPE_AUDIO: item->setIcon(0, QIcon(":/icons/audiosource.png")); break;
    case ICON_TYPE_ERROR: item->setIcon(0, QIcon::fromTheme("dialog-error")); break;
    }

	// refresh all clips
    QVector<Sequence*> sequences = panel_project->list_all_project_sequences();
    for (int i=0;i<sequences.size();i++) {
        Sequence* s = sequences.at(i);
        for (int j=0;j<s->clips.size();j++) {
            Clip* c = s->clips.at(j);
			if (c != NULL) {
				c->refresh();
            }
        }
	}

    // redraw clips
	update_ui(replace);

    panel_project->source_table->viewport()->update();
    deleteLater();
}

QString get_interlacing_name(int interlacing) {
	switch (interlacing) {
	case VIDEO_PROGRESSIVE: return "None (Progressive)";
	case VIDEO_TOP_FIELD_FIRST: return "Top Field First";
	case VIDEO_BOTTOM_FIELD_FIRST: return "Bottom Field First";
	default: return "Invalid";
	}
}

void update_footage_tooltip(QTreeWidgetItem *item, Media *media, QString error) {
	QString tooltip = "Name: " + media->name + "\nFilename: " + media->url + "\n";

	if (error.isEmpty()) {
		if (media->video_tracks.size() > 0) {
			tooltip += "Video Dimensions: ";
			for (int i=0;i<media->video_tracks.size();i++) {
				if (i > 0) {
					tooltip += ", ";
				}
				tooltip += QString::number(media->video_tracks.at(i)->video_width) + "x" + QString::number(media->video_tracks.at(i)->video_height);
			}
			tooltip += "\n";

			tooltip += "Frame Rate: ";
			for (int i=0;i<media->video_tracks.size();i++) {
				if (i > 0) {
					tooltip += ", ";
				}
				if (media->video_tracks.at(i)->video_interlacing != VIDEO_PROGRESSIVE) {
					tooltip += QString::number(media->video_tracks.at(i)->video_frame_rate);
				} else {
					tooltip += QString::number(media->video_tracks.at(i)->video_frame_rate * 2);
					tooltip += " fields (" + QString::number(media->video_tracks.at(i)->video_frame_rate) + " frames)";
				}
			}
			tooltip += "\n";

			tooltip += "Interlacing: ";
			for (int i=0;i<media->video_tracks.size();i++) {
				if (i > 0) {
					tooltip += ", ";
				}
				tooltip += get_interlacing_name(media->video_tracks.at(i)->video_interlacing);
			}
			tooltip += "\n";
		}

		if (media->audio_tracks.size() > 0) {
			tooltip += "Audio Frequency: ";
			for (int i=0;i<media->audio_tracks.size();i++) {
				if (i > 0) {
					tooltip += ", ";
				}
				tooltip += QString::number(media->audio_tracks.at(i)->audio_frequency);
			}
			tooltip += "\n";

			tooltip += "Audio Channels: ";
			for (int i=0;i<media->audio_tracks.size();i++) {
				if (i > 0) {
					tooltip += ", ";
				}
				tooltip += get_channel_layout_name(media->audio_tracks.at(i)->audio_channels, media->audio_tracks.at(i)->audio_layout);
			}
			// tooltip += "\n";
		}
	} else {

	}

	item->setToolTip(0, tooltip);
}
