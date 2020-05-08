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

#ifndef NODEVIEWFILTERDIALOG_H
#define NODEVIEWFILTERDIALOG_H

#include <QDialog>
#include <QGridLayout>

#include "node/node.h"

OLIVE_NAMESPACE_ENTER

class NodeViewFilterDialog : public QDialog
{
public:
  NodeViewFilterDialog(QWidget* parent = nullptr);

private:
  void RemoveRow(int row);

  void UpdateAddButtonPos();

  int row_count_;

  QGridLayout* filter_layout_;

  QPushButton* plus_btn_;

private slots:
  void AppendRow();

  void RemoveRowButtonClicked();

};

OLIVE_NAMESPACE_EXIT

#endif // NODEVIEWFILTERDIALOG_H
