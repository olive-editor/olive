#include "replaceclipmediadialog.h"

#include "ui/sourcetable.h"
#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "playback/playback.h"
#include "playback/cacher.h"
#include "project/footage.h"
#include "project/undo.h"
#include "project/media.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>

ReplaceClipMediaDialog::ReplaceClipMediaDialog(QWidget *parent, Media *old_media) :
	QDialog(parent),
	media(old_media)
{
	setWindowTitle(tr("Replace clips using \"%1\"").arg(old_media->get_name()));

	resize(300, 400);

	QVBoxLayout* layout = new QVBoxLayout(this);

	layout->addWidget(new QLabel(tr("Select which media you want to replace this media's clips with:"), this));

	tree = new QTreeView(this);

	layout->addWidget(tree);

	use_same_media_in_points = new QCheckBox(tr("Keep the same media in-points"), this);
	use_same_media_in_points->setChecked(true);
	layout->addWidget(use_same_media_in_points);

    QHBoxLayout* buttons = new QHBoxLayout();

	buttons->addStretch();

	QPushButton* replace_button = new QPushButton(tr("Replace"), this);
	connect(replace_button, SIGNAL(clicked(bool)), this, SLOT(replace()));
	buttons->addWidget(replace_button);

	QPushButton* cancel_button = new QPushButton(tr("Cancel"), this);
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(close()));
	buttons->addWidget(cancel_button);

	buttons->addStretch();

	layout->addLayout(buttons);

	tree->setModel(&project_model);
}

void ReplaceClipMediaDialog::replace() {
	QModelIndexList selected_items = tree->selectionModel()->selectedRows();
	if (selected_items.size() != 1) {
		QMessageBox::critical(
					this,
					tr("No media selected"),
					tr("Please select a media to replace with or click 'Cancel'."),
					QMessageBox::Ok
				);
	} else {
		Media* new_item = static_cast<Media*>(selected_items.at(0).internalPointer());
		if (media == new_item) {
			QMessageBox::critical(
						this,
						tr("Same media selected"),
						tr("You selected the same media that you're replacing. Please select a different one or click 'Cancel'."),
						QMessageBox::Ok
					);
		} else if (new_item->get_type() == MEDIA_TYPE_FOLDER) {
			QMessageBox::critical(
						this,
						tr("Folder selected"),
						tr("You cannot replace footage with a folder."),
						QMessageBox::Ok
					);
		} else {
			if (new_item->get_type() == MEDIA_TYPE_SEQUENCE && sequence == new_item->to_sequence()) {
				QMessageBox::critical(
							this,
							tr("Active sequence selected"),
							tr("You cannot insert a sequence into itself."),
							QMessageBox::Ok
						);
			} else {
				ReplaceClipMediaCommand* rcmc = new ReplaceClipMediaCommand(
							media,
							new_item,
							use_same_media_in_points->isChecked()
						);

				for (int i=0;i<sequence->clips.size();i++) {
					Clip* c = sequence->clips.at(i);
					if (c != nullptr && c->media == media) {
						rcmc->clips.append(c);
					}
				}

				undo_stack.push(rcmc);

				close();
			}

		}
	}
}
