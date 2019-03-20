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

#include "clippropertiesdialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>

#include "panels/panels.h"
#include "undo/undo.h"

ClipPropertiesDialog::ClipPropertiesDialog(QWidget *parent, QVector<Clip *> clips) :
  QDialog(parent)
{
  setWindowTitle((clips.size() == 1) ?
                   tr("\"%1\" Properties").arg(clips.at(0)->name()) :
                   tr("Multiple Clip Properties"));

  clips_ = clips;

  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

  // Clip Name field
  layout->addWidget(new QLabel(tr("Name:")), row, 0);

  clip_name_field_ = new QLineEdit();

  layout->addWidget(clip_name_field_, row, 1);

  row++;

  // Clip Duration field
  layout->addWidget(new QLabel(tr("Duration:")), row, 0);

  duration_field_ = new LabelSlider();
  duration_field_->SetDisplayType(LabelSlider::FrameNumber);
  duration_field_->SetMinimum(1);
  layout->addWidget(duration_field_, row, 1);

  row++;

  // Dialog buttons (OK and Cancel)
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons, row, 0, 1, 2);

  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

  // analyze list of clips for default values

  bool all_clips_have_same_name = true;
  bool all_clips_have_same_duration = true;

  for (int i=1;i<clips.size();i++) {
    if (clips.at(i-1)->name() != clips.at(i)->name()) {
      all_clips_have_same_name = false;
    }
    if (clips.at(i-1)->length() != clips.at(i)->length()) {
      all_clips_have_same_duration = false;
    }
  }

  Clip* first_clip = clips_.first();

  if (all_clips_have_same_name) {
    // if there's only one clip selected, set all defaults to that clip's properties
    clip_name_field_->setText(first_clip->name());
  } else {
    // if there are multiple clips, use different properties
    clip_name_field_->setPlaceholderText(tr("(multiple)"));
  }

  // it's assumed all the clips come from the same sequence
  duration_field_->SetFrameRate(first_clip->sequence->frame_rate);

  if (all_clips_have_same_duration) {
    duration_field_->SetDefault(first_clip->length());
    duration_field_->SetValue(first_clip->length());
    duration_field_->SetMaximum(first_clip->media_length());
  } else {
    duration_field_->SetDefault(qSNaN());
    duration_field_->SetValue(qSNaN());
  }
}

void ClipPropertiesDialog::accept()
{
  const QString& clip_name = clip_name_field_->text();
  double clip_duration = duration_field_->value();

  ComboAction* ca = new ComboAction();

  for (int i=0;i<clips_.size();i++) {
    Clip* clip = clips_.at(i);

    if (!clip_name.isEmpty()) {
      ca->append(new RenameClipCommand(clip, clip_name));
    }

    if (!qIsNaN(clip_duration)) {
      clip->move(ca,
                 clip->timeline_in(),
                 clip->timeline_in() + qRound(clip_duration),
                 clip->clip_in(),
                 clip->track());
    }
  }

  if (ca->hasActions()) {
    olive::UndoStack.push(ca);
    update_ui(false);
  } else {
    delete ca;
  }

  QDialog::accept();
}
