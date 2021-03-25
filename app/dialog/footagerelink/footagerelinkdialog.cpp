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

#include "footagerelinkdialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

namespace olive {

FootageRelinkDialog::FootageRelinkDialog(const QVector<Footage *> &footage, QWidget* parent) :
  QDialog(parent),
  footage_(footage)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel("The following files couldn't be found. Clips using them will be "
                               "unplayable until they're relinked."));

  table_ = new QTreeWidget();

  table_->setColumnCount(3);
  table_->setHeaderLabels({tr("Footage"), tr("Filename"), tr("Actions")});
  table_->setRootIsDecorated(false);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->header()->setSectionsMovable(false);

  // Prefer stretching URL column (QHeaderView defaults to stretching the last column, which in
  // our case is just a browse button)
  table_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  table_->header()->setStretchLastSection(false);

  for (int i=0; i<footage.size(); i++) {
    Footage* f = footage.at(i);
    QTreeWidgetItem* item = new QTreeWidgetItem();

    QWidget* item_actions = new QWidget();
    QHBoxLayout* item_actions_layout = new QHBoxLayout(item_actions);
    QPushButton* item_browse_btn = new QPushButton(tr("Browse"));
    item_browse_btn->setProperty("index", i);
    connect(item_browse_btn, &QPushButton::clicked, this, &FootageRelinkDialog::BrowseForFootage);
    item_actions_layout->addWidget(item_browse_btn);

    item->setIcon(0, f->icon());
    item->setText(0, f->GetLabel());
    item->setText(1, f->filename());

    table_->addTopLevelItem(item);

    table_->setItemWidget(item, 2, item_actions);
  }

  layout->addWidget(table_);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &FootageRelinkDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &FootageRelinkDialog::reject);
  layout->addWidget(buttons);

  setWindowTitle(tr("Relink Footage"));


}

void FootageRelinkDialog::UpdateFootageItem(int index)
{
  Footage* f = footage_.at(index);
  QTreeWidgetItem* item = table_->topLevelItem(index);
  item->setIcon(0, f->icon());
  item->setText(1, f->filename());
}

void FootageRelinkDialog::BrowseForFootage()
{
  int index = sender()->property("index").toInt();
  Footage* f = footage_.at(index);

  QFileInfo info(f->filename());

  QString new_fn = QFileDialog::getOpenFileName(this,
                                                tr("Relink \"%1\"").arg(f->GetLabel()),
                                                info.absolutePath(),
                                                QStringLiteral("%1;;%2 (**)").arg(info.fileName(), tr("All Files")));

  // We received a new filename
  if (!new_fn.isEmpty()) {
    // Store original dir since we might be able to use this to find other files
    QDir original_dir = info.dir();
    QDir new_dir = QFileInfo(new_fn).dir();

    // Set new filename since this was set manually by the user
    f->set_filename(new_fn);

    if (Footage::CompareFootageToItsFilename(f)) {
      // Set footage to valid and update icon
      f->SetValid();

      // Update item visually
      UpdateFootageItem(index);
    }

    // Check all other footage files for matches
    for (int it=0; it<footage_.size(); it++) {
      Footage* other_footage = footage_.at(it);

      // Ignore current footage file and footage that's already valid of course
      if (index != it && !other_footage->IsValid()) {
        // Get footage path relative to original directory
        QString relative_to_original = original_dir.relativeFilePath(other_footage->filename());
        QString absolute_to_new = new_dir.filePath(relative_to_original);

        // Check if file exists
        if (QFileInfo::exists(absolute_to_new)
            && Footage::CompareFootageToFile(other_footage, absolute_to_new)) {
          other_footage->set_filename(absolute_to_new);
          other_footage->SetValid();
          UpdateFootageItem(it);
        }
      }
    }
  }

  // Check where the next invalid footage is. If there is none, accept automatically. Otherwise,
  // jump to that footage so the user knows where it is.
  int next_invalid = -1;
  for (int i=0; i<footage_.size(); i++) {
    if (!footage_.at(i)->IsValid()) {
      next_invalid = i;
      break;
    }
  }

  if (next_invalid == -1) {
    // No more invalid footage, just accept
    this->accept();
  } else {
    // Jump to next invalid footage
    QModelIndex idx = table_->model()->index(next_invalid, 0);
    table_->selectionModel()->select(idx,
                                     QItemSelectionModel::Select | QItemSelectionModel::Rows);
    table_->scrollTo(idx);

  }
}

}
