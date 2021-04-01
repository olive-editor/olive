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

#include "autorecoverydialog.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "core.h"

namespace olive {

#define super QDialog

AutoRecoveryDialog::AutoRecoveryDialog(const QString &message, const QStringList &recoveries, bool autocheck_latest, QWidget* parent) :
  QDialog(parent)
{
  Init(message);

  PopulateTree(recoveries, autocheck_latest);
}

void AutoRecoveryDialog::accept()
{
  foreach (QTreeWidgetItem* checkable, checkable_items_) {
    if (checkable->checkState(0) == Qt::Checked) {
      QString filename = checkable->data(0, kFilenameRole).toString();
      Core::instance()->OpenRecoveryProject(filename);
    }
  }

  super::accept();
}

void AutoRecoveryDialog::Init(const QString& header_text)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  setWindowTitle(tr("Auto-Recovery"));

  layout->addWidget(new QLabel(header_text));

  tree_widget_ = new QTreeWidget();
  tree_widget_->setHeaderHidden(true);
  layout->addWidget(tree_widget_);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->button(QDialogButtonBox::Ok)->setText(tr("Load"));
  connect(buttons, &QDialogButtonBox::accepted, this, &AutoRecoveryDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &AutoRecoveryDialog::reject);
  layout->addWidget(buttons);
}

void AutoRecoveryDialog::PopulateTree(const QStringList& recoveries, bool autocheck_latest)
{
  // Each entry in `recoveries` is a directory with 1+ recovery projects in it
  foreach (const QString& recovery_folder, recoveries) {
    QDir recovery_dir(recovery_folder);

    QString pretty_name;
    {
      // Retrieve pretty name
      QFile pretty_name_file(recovery_dir.filePath(QStringLiteral("realname.txt")));
      if (pretty_name_file.open(QFile::ReadOnly)) {
        // Read pretty name that we should have written in the autorecovery process
        pretty_name = QString::fromUtf8(pretty_name_file.readAll());
        pretty_name_file.close();
      }

      if (pretty_name.isEmpty()) {
        // Fallback to just the UUID. While it won't mean much to the user, it's better than nothing.
        pretty_name = recovery_dir.dirName();
      }
    }

    QTreeWidgetItem* top_level = new QTreeWidgetItem(tree_widget_);
    top_level->setText(0, pretty_name);

    {
      // Populate with recoveries
      QStringList entries = recovery_dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
      for (int i=0; i<entries.size(); i++) {
        const QString& entry = entries.at(i);

        if (entry.endsWith(QStringLiteral(".ove"), Qt::CaseInsensitive)) {
          QTreeWidgetItem* entry_item = new QTreeWidgetItem(top_level);

          bool ok;
          qint64 recovery_time = entry.left(entry.indexOf('.')).toLongLong(&ok);
          QString entry_name;

          if (ok) {
            // Set as time/date of recovery
            entry_name = QDateTime::fromSecsSinceEpoch(recovery_time).toString();
          } else {
            // Fallback if we couldn't discern a date from this
            entry_name = entry;
          }

          entry_item->setText(0, entry_name);
          entry_item->setData(0, kFilenameRole, recovery_dir.filePath(entry));

          // Allow to be checked, auto-checking the first entry
          entry_item->setCheckState(0, (autocheck_latest && top_level->childCount() == 1) ? Qt::Checked : Qt::Unchecked);

          checkable_items_.append(entry_item);
        }
      }
    }
  }
}

}
