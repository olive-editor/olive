/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "core.h"
#include "render/colormanager.h"
#include "streamproperties/audiostreamproperties.h"
#include "streamproperties/videostreamproperties.h"

OLIVE_NAMESPACE_ENTER

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

  int first_usable_stream = -1;

  for (int i=0;i<footage_->streams().size();i++) {
    StreamPtr stream = footage_->stream(i);

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

    if (first_usable_stream == -1
        && (stream->type() == Stream::kVideo
            || stream->type() == Stream::kAudio)) {
      first_usable_stream = i;
    }
  }

  row++;

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons, row, 0, 1, 2);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(track_list, &QListWidget::currentRowChanged, stacked_widget_, &QStackedWidget::setCurrentIndex);

  // Auto-select first item that actually has properties
  if (first_usable_stream >= 0) {
    track_list->setCurrentRow(first_usable_stream);
  }
  track_list->setFocus();
}

void FootagePropertiesDialog::accept() {
  // Perform sanity check on all pages
  for (int i=0;i<stacked_widget_->count();i++) {
    if (!static_cast<StreamProperties*>(stacked_widget_->widget(i))->SanityCheck()) {
      // Switch to the failed panel in question
      stacked_widget_->setCurrentIndex(i);

      // Do nothing (it's up to the property panel itself to throw the error message)
      return;
    }
  }

  QUndoCommand* command = new QUndoCommand();

  if (footage_->name() != footage_name_field_->text()) {
    new FootageChangeCommand(footage_,
                             footage_name_field_->text(),
                             command);
  }

  for (int i=0;i<footage_->streams().size();i++) {
    bool stream_enabled = (track_list->item(i)->checkState() == Qt::Checked);

    if (footage_->stream(i)->enabled() != stream_enabled) {
      new StreamEnableChangeCommand(footage_->stream(i),
                                    stream_enabled,
                                    command);
    }
  }

  for (int i=0;i<stacked_widget_->count();i++) {
    static_cast<StreamProperties*>(stacked_widget_->widget(i))->Accept(command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  QDialog::accept();
}

FootagePropertiesDialog::FootageChangeCommand::FootageChangeCommand(Footage *footage, const QString &name, QUndoCommand* command) :
  UndoCommand(command),
  footage_(footage),
  new_name_(name)
{
}

Project *FootagePropertiesDialog::FootageChangeCommand::GetRelevantProject() const
{
  return footage_->project();
}

void FootagePropertiesDialog::FootageChangeCommand::redo_internal()
{
  old_name_ = footage_->name();

  footage_->set_name(new_name_);
}

void FootagePropertiesDialog::FootageChangeCommand::undo_internal()
{
  footage_->set_name(old_name_);
}

FootagePropertiesDialog::StreamEnableChangeCommand::StreamEnableChangeCommand(StreamPtr stream, bool enabled, QUndoCommand *command) :
  UndoCommand(command),
  stream_(stream),
  old_enabled_(stream->enabled()),
  new_enabled_(enabled)
{
}

Project *FootagePropertiesDialog::StreamEnableChangeCommand::GetRelevantProject() const
{
  return stream_->footage()->project();
}

void FootagePropertiesDialog::StreamEnableChangeCommand::redo_internal()
{
  stream_->set_enabled(new_enabled_);
}

void FootagePropertiesDialog::StreamEnableChangeCommand::undo_internal()
{
  stream_->set_enabled(old_enabled_);
}

OLIVE_NAMESPACE_EXIT
