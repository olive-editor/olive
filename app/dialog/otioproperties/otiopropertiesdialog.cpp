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

#include "otiopropertiesdialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "dialog/sequence/sequence.h"
#include "core.h"

OLIVE_NAMESPACE_ENTER

OTIOPropertiesDialog::OTIOPropertiesDialog(QList<SequencePtr> sequences, ProjectPtr active_project, QWidget* parent)
    :
	QDialog(parent),
    sequences_(sequences)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel("Change settings for each OTIO sequence to be loaded"));

  table_ = new QTreeWidget();
  table_->setColumnCount(2);
  table_->setHeaderLabels({tr("Sequence"), tr("Actions")});
  table_->setRootIsDecorated(false);

  for (int i = 0; i < sequences.size(); i++) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    SequencePtr s = sequences.at(i);

    QWidget* item_actions = new QWidget();
    QHBoxLayout* item_actions_layout = new QHBoxLayout(item_actions);
    QPushButton* item_settings_btn = new QPushButton(tr("Settings"));
    item_settings_btn->setProperty("index", i);
    connect(item_settings_btn, &QPushButton::clicked, this, &OTIOPropertiesDialog::SetupSequence);
    item_actions_layout->addWidget(item_settings_btn);

    item->setText(0, s->name());

    table_->addTopLevelItem(item);

    table_->setItemWidget(item, 1, item_actions);
  }

  layout->addWidget(table_);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttons, &QDialogButtonBox::accepted, this, &OTIOPropertiesDialog::accept);
  layout->addWidget(buttons);

  setWindowTitle(tr("Load OTIO File"));
}

void OTIOPropertiesDialog::SetupSequence()
{
  int index = sender()->property("index").toInt();
  SequencePtr s = sequences_.at(index);
  SequenceDialog sd(s.get(), SequenceDialog::kNew);
  sd.SetUndoable(false);
  sd.exec();
}

OLIVE_NAMESPACE_EXIT
