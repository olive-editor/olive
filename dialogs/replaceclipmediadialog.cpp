#include "replaceclipmediadialog.h"

#include "ui/sourcetable.h"
#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "playback/playback.h"
#include "playback/cacher.h"
#include "io/media.h"

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>

ReplaceClipMediaDialog::ReplaceClipMediaDialog(QWidget *parent, SourceTable *table, QTreeWidgetItem *old_media) :
	QDialog(parent),
	source_table(table),
	media(old_media)
{
	resize(300, 400);

	QVBoxLayout* layout = new QVBoxLayout();

	layout->addWidget(new QLabel("Select which media you want to replace this media's clips with:"));

	tree = new QTreeWidget();
	tree->setHeaderHidden(true);

	layout->addWidget(tree);

	use_same_media_in_points = new QCheckBox("Keep the same media in points");
	use_same_media_in_points->setChecked(true);
	layout->addWidget(use_same_media_in_points);

	QHBoxLayout* buttons = new QHBoxLayout();

	buttons->addStretch();

	QPushButton* replace_button = new QPushButton("Replace");
	connect(replace_button, SIGNAL(clicked(bool)), this, SLOT(replace()));
	buttons->addWidget(replace_button);

	QPushButton* cancel_button = new QPushButton("Cancel");
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(close()));
	buttons->addWidget(cancel_button);

	buttons->addStretch();

	layout->addLayout(buttons);

	setLayout(layout);

	copy_tree(NULL, NULL);
}

void ReplaceClipMediaDialog::replace() {
	if (tree->selectedItems().size() != 1) {
		QMessageBox::critical(this, "No media selected", "Please select a media to replace with or click 'Cancel'.", QMessageBox::Ok);
	} else {
		QTreeWidgetItem* selected_item = tree->selectedItems().at(0);
		QTreeWidgetItem* new_item = reinterpret_cast<QTreeWidgetItem*>(selected_item->data(0, Qt::UserRole + 1).value<quintptr>());
		if (media == new_item) {
			QMessageBox::critical(this, "Same media selected", "You selected the same media that you're replacing. Please select a different one or click 'Cancel'.", QMessageBox::Ok);
		} else if (get_type_from_tree(new_item) == MEDIA_TYPE_FOLDER) {
			QMessageBox::critical(this, "Folder selected", "You cannot replace footage with a folder.", QMessageBox::Ok);
		} else {
			if (get_type_from_tree(new_item) == MEDIA_TYPE_SEQUENCE && sequence == get_sequence_from_tree(new_item)) {
				QMessageBox::critical(this, "Active sequence selected", "You cannot insert a sequence into itself.", QMessageBox::Ok);
			} else {
				void* old_media = get_media_from_tree(media);
				void* new_media = get_media_from_tree(new_item);

				// TODO not undoable yet
				bool changed = false;
				for (int i=0;i<sequence->clip_count();i++) {
					Clip* c = sequence->get_clip(i);
					if (c != NULL && c->media == old_media) {
						if (c->open) {
							close_clip(c);
							c->cacher->wait();
						}

						if (!use_same_media_in_points->isChecked()) {
							c->clip_in = 0;
						}

						c->media = new_media;

						if (get_type_from_tree(new_item) == MEDIA_TYPE_FOOTAGE) {
							// TODO: the media streams may be invalid, here we have a BASIC heuristic for getting the right ones that COULD BE BETTER
							Media* casted_new_media = static_cast<Media*>(new_media);
							c->media_stream = (c->track < 0) ? casted_new_media->video_tracks.at(0)->file_index : casted_new_media->audio_tracks.at(0)->file_index;
						}

						c->refresh();
						changed = true;
					}
				}
				if (changed) {
					panel_timeline->redraw_all_clips(true);
				}
				close();
			}

		}
	}
}

void ReplaceClipMediaDialog::copy_tree(QTreeWidgetItem* parent, QTreeWidgetItem* target) {
	QVector<QTreeWidgetItem*> items;

	if (parent == NULL) {
		for (int i=0;i<source_table->topLevelItemCount();i++) {
			items.append(source_table->topLevelItem(i));
		}
	} else {
		for (int i=0;i<parent->childCount();i++) {
			items.append(parent->child(i));
		}
	}

	for (int i=0;i<items.size();i++) {
		QTreeWidgetItem* original = items.at(i);
		QTreeWidgetItem* item = new QTreeWidgetItem();
		item->setText(0, original->text(0));
		item->setData(0, Qt::UserRole + 1, reinterpret_cast<quintptr>(original));

		if (target == NULL) {
			tree->addTopLevelItem(item);
		} else {
			target->addChild(item);
		}

		if (original->childCount() > 0) {
			copy_tree(original, item);
		}
	}
}
