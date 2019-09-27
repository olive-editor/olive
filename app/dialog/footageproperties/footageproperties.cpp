/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "footageproperties.h"

#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QTreeWidgetItem>
#include <QGroupBox>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>

#include "streamproperties/audiostreamproperties.h"
#include "streamproperties/videostreamproperties.h"
#include "undo/undostack.h"

FootagePropertiesDialog::FootagePropertiesDialog(QWidget *parent, Footage *footage) :
  QDialog(parent),
  footage_(footage)
{
  QGridLayout* layout = new QGridLayout(this);

  setWindowTitle(tr("\"%1\" Properties").arg(footage_->name()));
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  int row = 0;

  layout->addWidget(new QLabel(tr("Name:")), row, 0);

  footage_name_field_ = new QLineEdit(footage_->name());
  layout->addWidget(footage_name_field_, row, 1);
  row++;

  layout->addWidget(new QLabel(tr("Tracks:")), row, 0, 1, 2);
  row++;

  track_list = new QListWidget();
  layout->addWidget(track_list, row, 0, 1, 2);

  row++;

  stacked_widget_ = new QStackedWidget();
  layout->addWidget(stacked_widget_, row, 0, 1, 2);

  foreach (StreamPtr stream, footage_->streams()) {
    QListWidgetItem* item = new QListWidgetItem(stream->description(), track_list);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(stream->enabled() ? Qt::Checked : Qt::Unchecked);
    track_list->addItem(item);

    switch (stream->type()) {
    case Stream::kVideo:
      stacked_widget_->addWidget(new VideoStreamProperties(std::static_pointer_cast<VideoStream>(stream)));
      break;
    case Stream::kAudio:
      stacked_widget_->addWidget(new AudioStreamProperties(std::static_pointer_cast<AudioStream>(stream)));
      break;
    default:
      stacked_widget_->addWidget(new StreamProperties());
    }
  }

  row++;

  /*if (f->video_tracks.size() > 0) {
    // frame conforming
    if (!f->video_tracks.at(0).infinite_length) {
      layout->addWidget(new QLabel(tr("Conform to Frame Rate:"), this), row, 0);
      conform_fr = new QDoubleSpinBox(this);
      conform_fr->setMinimum(0.01);
      conform_fr->setValue(f->video_tracks.at(0).video_frame_rate * f->speed);
      layout->addWidget(conform_fr, row, 1);
    }

    row++;

    // premultiplied alpha mode
    premultiply_alpha_setting = new QCheckBox(tr("Alpha is Premultiplied"), this);
    premultiply_alpha_setting->setChecked(f->alpha_is_associated);
    layout->addWidget(premultiply_alpha_setting, row, 0);

    row++;

    // deinterlacing mode
    interlacing_box = new QComboBox(this);
    interlacing_box->addItem(
          tr("Auto (%1)").arg(
            Footage::get_interlacing_name(f->video_tracks.at(0).video_auto_interlacing)
            )
          );
    interlacing_box->addItem(Footage::get_interlacing_name(VIDEO_PROGRESSIVE));
    interlacing_box->addItem(Footage::get_interlacing_name(VIDEO_TOP_FIELD_FIRST));
    interlacing_box->addItem(Footage::get_interlacing_name(VIDEO_BOTTOM_FIELD_FIRST));

    interlacing_box->setCurrentIndex(
          (f->video_tracks.at(0).video_auto_interlacing == f->video_tracks.at(0).video_interlacing)
          ? 0
          : f->video_tracks.at(0).video_interlacing + 1);

    layout->addWidget(new QLabel(tr("Interlacing:"), this), row, 0);
    layout->addWidget(interlacing_box, row, 1);

    row++;

    input_color_space = new QComboBox(this);

    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    QString footage_colorspace = f->Colorspace();

    for (int i=0;i<config->getNumColorSpaces();i++) {
      QString colorspace = config->getColorSpaceNameByIndex(i);

      input_color_space->addItem(colorspace);

      if (colorspace == footage_colorspace) {
        input_color_space->setCurrentIndex(i);
      }
    }

    layout->addWidget(new QLabel(tr("Color Space:")), row, 0);
    layout->addWidget(input_color_space, row, 1);

    row++;

  }
  */

  connect(track_list, SIGNAL(currentRowChanged(int)), stacked_widget_, SLOT(setCurrentIndex(int)));

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons, row, 0, 1, 2);

  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void FootagePropertiesDialog::accept() {
  /*Footage* f = item->to_footage();

  ComboAction* ca = new ComboAction();

  // set track enable
  for (int i=0;i<track_list->count();i++) {
    QListWidgetItem* item = track_list->item(i);
    const QVariant& data = item->data(Qt::UserRole+1);
    if (!data.isNull()) {
      int index = data.toInt();
      bool found = false;
      for (int j=0;j<f->video_tracks.size();j++) {
        if (f->video_tracks.at(j).file_index == index) {
          f->video_tracks[j].enabled = (item->checkState() == Qt::Checked);
          found = true;
          break;
        }
      }
      if (!found) {
        for (int j=0;j<f->audio_tracks.size();j++) {
          if (f->audio_tracks.at(j).file_index == index) {
            f->audio_tracks[j].enabled = (item->checkState() == Qt::Checked);
            break;
          }
        }
      }
    }
  }

  bool refresh_clips = false;

  // set interlacing
  if (f->video_tracks.size() > 0) {
    if (interlacing_box->currentIndex() > 0) {
      ca->append(new SetInt(&f->video_tracks[0].video_interlacing, interlacing_box->currentIndex() - 1));
    } else {
      ca->append(new SetInt(&f->video_tracks[0].video_interlacing, f->video_tracks.at(0).video_auto_interlacing));
    }

    // set frame rate conform
    if (!f->video_tracks.at(0).infinite_length) {
      if (!qFuzzyCompare(conform_fr->value(), f->video_tracks.at(0).video_frame_rate)) {
        ca->append(new SetDouble(&f->speed, f->speed, conform_fr->value()/f->video_tracks.at(0).video_frame_rate));
        refresh_clips = true;
      }
    }

    // set premultiplied alpha
    f->alpha_is_associated = premultiply_alpha_setting->isChecked();
  }

  f->SetColorspace(input_color_space->currentText());

  // set name
  MediaRename* mr = new MediaRename(item, name_box->text());

  ca->append(mr);
  ca->appendPost(new CloseAllClipsCommand());
  ca->appendPost(new UpdateFootageTooltip(item));
  if (refresh_clips) {
    ca->appendPost(new RefreshClips(item));
  }
  ca->appendPost(new UpdateViewer());

  olive::undo_stack.push(ca);*/

  QUndoCommand* command = new QUndoCommand();

  if (footage_->name() != footage_name_field_->text()) {
    new FootageChangeCommand(footage_,
                             footage_name_field_->text(),
                             command);
  }

  for (int i=0;i<footage_->streams().size();i++) {
    bool stream_enabled = (track_list->item(i)->checkState() == Qt::Checked);

    if (footage_->stream(i)->enabled() == stream_enabled) {
      new StreamEnableChangeCommand(footage_->stream(i),
                                    stream_enabled,
                                    command);
    }
  }

  for (int i=0;i<stacked_widget_->count();i++) {
    static_cast<StreamProperties*>(stacked_widget_->widget(i))->Accept(command);
  }

  olive::undo_stack.pushIfHasChildren(command);

  QDialog::accept();
}

FootagePropertiesDialog::FootageChangeCommand::FootageChangeCommand(Footage *footage, const QString &name, QUndoCommand* command) :
  QUndoCommand(command),
  footage_(footage),
  new_name_(name)
{
}

void FootagePropertiesDialog::FootageChangeCommand::redo()
{
  old_name_ = footage_->name();

  footage_->set_name(new_name_);
}

void FootagePropertiesDialog::FootageChangeCommand::undo()
{
  footage_->set_name(old_name_);
}

FootagePropertiesDialog::StreamEnableChangeCommand::StreamEnableChangeCommand(StreamPtr stream, bool enabled, QUndoCommand *command) :
  QUndoCommand(command),
  stream_(stream),
  new_enabled_(enabled)
{
}

void FootagePropertiesDialog::StreamEnableChangeCommand::redo()
{
  old_enabled_ = stream_->enabled();

  stream_->set_enabled(new_enabled_);
}

void FootagePropertiesDialog::StreamEnableChangeCommand::undo()
{
  stream_->set_enabled(old_enabled_);
}
