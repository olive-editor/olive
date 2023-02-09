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
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "core.h"
#include "dialog/sequence/sequence.h"

namespace olive {

OTIOPropertiesDialog::OTIOPropertiesDialog(const QList<Sequence*>& sequences, Project* active_project, QWidget* parent)
    :
    QDialog(parent),
    sequences_(sequences)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QLabel *msg = new QLabel(tr("OpenTimelineIO files do not store sequence parameters (resolution, frame rate, etc.)\n\n"
                              "Please set the correct parameters on the sequences below (they have been set to your default sequence parameters as a starting point)."));
  msg->setWordWrap(true);
  layout->addWidget(msg);

  table_ = new QTreeWidget();
  table_->setColumnCount(2);
  table_->setHeaderLabels({tr("Sequence"), tr("Actions")});
  table_->setRootIsDecorated(false);

  for (int i = 0; i < sequences.size(); i++) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    Sequence* s = sequences.at(i);

    QWidget* item_actions = new QWidget();
    QHBoxLayout* item_actions_layout = new QHBoxLayout(item_actions);
    QPushButton* item_settings_btn = new QPushButton(tr("Settings"));
    item_settings_btn->setProperty("index", i);
    connect(item_settings_btn, &QPushButton::clicked, this, &OTIOPropertiesDialog::SetupSequence);
    item_actions_layout->addWidget(item_settings_btn);

    item->setText(0, s->GetLabel());

    table_->addTopLevelItem(item);

    table_->setItemWidget(item, 1, item_actions);
  }

  // Stretch first column to take up as much space as possible, and second column to take as little
  table_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  table_->header()->setSectionResizeMode(1, QHeaderView::Fixed);
  table_->header()->setStretchLastSection(false);

  layout->addWidget(table_);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &OTIOPropertiesDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &OTIOPropertiesDialog::reject);
  layout->addWidget(buttons);

  setWindowTitle(tr("Load OpenTimelineIO Project"));
}

void OTIOPropertiesDialog::SetupSequence() {
  int index = sender()->property("index").toInt();
  Sequence* s = sequences_.at(index);
  SequenceDialog sd(s, SequenceDialog::kNew);
  sd.SetUndoable(false);
  sd.exec();
}

}  // namespace olive
