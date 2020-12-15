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

#ifndef FOOTAGERELINKDIALOG_H
#define FOOTAGERELINKDIALOG_H

#include <QDialog>
#include <QTreeWidget>

#include "project/item/footage/footage.h"

namespace olive {

class FootageRelinkDialog : public QDialog
{
  Q_OBJECT
public:
  FootageRelinkDialog(const QVector<Footage*>& footage, QWidget* parent = nullptr);

private:
  void UpdateFootageItem(int index);

  QTreeWidget* table_;

  QVector<Footage*> footage_;

private slots:
  void BrowseForFootage();

};

}

#endif // FOOTAGERELINKDIALOG_H
