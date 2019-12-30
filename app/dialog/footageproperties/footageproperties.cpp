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

#include "core.h"
#include "render/colormanager.h"
#include "streamproperties/audiostreamproperties.h"
#include "streamproperties/videostreamproperties.h"

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

  connect(track_list, SIGNAL(currentRowChanged(int)), stacked_widget_, SLOT(setCurrentIndex(int)));

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons, row, 0, 1, 2);

  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void FootagePropertiesDialog::accept() {
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
  old_enabled_(stream->enabled()),
  new_enabled_(enabled)
{
}

void FootagePropertiesDialog::StreamEnableChangeCommand::redo()
{
  stream_->set_enabled(new_enabled_);
}

void FootagePropertiesDialog::StreamEnableChangeCommand::undo()
{
  stream_->set_enabled(old_enabled_);
}
