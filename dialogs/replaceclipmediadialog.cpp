/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "replaceclipmediadialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

#include "panels/panels.h"
#include "undo/undostack.h"
#include "rendering/cacher.h"
#include "undo/undo.h"

ReplaceClipMediaDialog::ReplaceClipMediaDialog(QWidget *parent, Media* old_media) :
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
  connect(replace_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
  buttons->addWidget(replace_button);

  QPushButton* cancel_button = new QPushButton(tr("Cancel"), this);
  connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
  buttons->addWidget(cancel_button);

  buttons->addStretch();

  layout->addLayout(buttons);

  tree->setModel(&olive::project_model);
}

void ReplaceClipMediaDialog::accept() {
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

      SequencePtr top_sequence = Timeline::GetTopSequence();

      if (new_item->get_type() == MEDIA_TYPE_SEQUENCE && top_sequence == new_item->to_sequence()) {
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

        QVector<Clip*> all_clips = top_sequence->GetAllClips();
        for (int i=0;i<all_clips.size();i++) {
          Clip* c = all_clips.at(i);
          if (c->media() == media) {
            rcmc->clips.append(c);
          }
        }

        olive::undo_stack.push(rcmc);

        QDialog::accept();
      }

    }
  }
}
