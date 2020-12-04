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
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "project/project.h"
#include "node/input/media/media.h"

namespace olive {

FootageRelinkDialog::FootageRelinkDialog(const QList<FootagePtr>& footage, QWidget* parent) :
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

  for (int i=0; i<footage.size(); i++) {
    FootagePtr f = footage.at(i);
    QTreeWidgetItem* item = new QTreeWidgetItem();

    QWidget* item_actions = new QWidget();
    QHBoxLayout* item_actions_layout = new QHBoxLayout(item_actions);
    QPushButton* item_browse_btn = new QPushButton(tr("Browse"));
    item_browse_btn->setProperty("index", i);
    connect(item_browse_btn, &QPushButton::clicked, this, &FootageRelinkDialog::BrowseForFootage);
    item_actions_layout->addWidget(item_browse_btn);

    item->setIcon(0, f->icon());
    item->setText(0, f->name());
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

void FootageRelinkDialog::BrowseForFootage()
{
  int index = sender()->property("index").toInt();
  FootagePtr f = footage_.at(index);

  QFileInfo info(f->filename());

  QString new_fn = QFileDialog::getOpenFileName(this,
                                                tr("Relink \"%1\"").arg(f->name()),
                                                info.absolutePath(),
                                                QStringLiteral("%1;;%2 (**)").arg(info.fileName(), tr("All Files")));

  if (!new_fn.isEmpty()) {
    f->set_filename(new_fn);

    if (Footage::CompareFootageToItsFilename(f)) {
      // Set footage to valid and update icon
      f->SetValid();
      relink_.append(f);
      AttemptAutoRelink(f->filename());

      QTreeWidgetItem* item = table_->topLevelItem(index);
      item->setIcon(0, f->icon());
      item->setText(1, f->filename());

      // Loop through all nodes to see if any need to be re-linked to this footage
      QList<ItemPtr> sequences = f->project()->get_items_of_type(Item::kSequence);

      foreach (ItemPtr s, sequences) {
        const QList<Node*>& nodes = static_cast<Sequence*>(s.get())->nodes();
        foreach (Node* n, nodes) {
          if (n->IsMedia()) {
            MediaInput* media_node = static_cast<MediaInput*>(n);

            StreamPtr node_stream = media_node->stream();
            foreach (FootagePtr footage, relink_) {
              if (node_stream->footage() == footage.get()) {
                foreach (StreamPtr str, footage->streams()) {
                  // Are there cases this will miss? E.g. loadingg proxies with no audio stream or something?
                  if (node_stream->index() == str.get()->index()) {
                    media_node->SetStream(str);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void FootageRelinkDialog::AttemptAutoRelink(QString valid_path)
{
  foreach(FootagePtr f, footage_) {
    if (!f->IsValid()) {
      QFileInfo f_filepath(f->filename());
      QFileInfo valid_file(valid_path);
      QFileInfo test_file;
      QString test_path;

      // Try same folder
      // Needs to handle OS path splitters "/" vs "\"
      test_path = valid_file.path() + "/" + f_filepath.fileName();
      test_file.setFile(test_path);

      if (test_file.exists()) {
        f->set_filename(test_path);
        if (Footage::CompareFootageToItsFilename(f)) {
          f->SetValid();
          relink_.append(f);
          foreach(QTreeWidgetItem * item, table_->findItems(f_filepath.filePath(), Qt::MatchExactly, 1)) {
            item->setIcon(0, f->icon());
            item->setText(1, f->filename());
          }
        } else {
          f->set_filename(f_filepath.absoluteFilePath());
        }
      }
    }
  }
}

}
