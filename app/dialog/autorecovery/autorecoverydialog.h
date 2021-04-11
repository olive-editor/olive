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

#ifndef AUTORECOVERYDIALOG_H
#define AUTORECOVERYDIALOG_H

#include <QDialog>
#include <QTreeWidget>

#include "common/define.h"

namespace olive {

class AutoRecoveryDialog : public QDialog
{
  Q_OBJECT
public:
  AutoRecoveryDialog(const QString& message, const QStringList& recoveries, bool autocheck_latest, QWidget* parent);

public slots:
  virtual void accept() override;

private:
  void Init(const QString &header_text);

  void PopulateTree(const QStringList &recoveries, bool autocheck);

  QTreeWidget* tree_widget_;

  QVector<QTreeWidgetItem*> checkable_items_;

  enum DataRole {
    kFilenameRole = Qt::UserRole
  };

};

}

#endif // AUTORECOVERYDIALOG_H
