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

#ifndef FOOTAGECOMBOBOX_H
#define FOOTAGECOMBOBOX_H

#include <QComboBox>
#include <QMenu>

#include "project/item/footage/footage.h"
#include "project/project.h"

OLIVE_NAMESPACE_ENTER

class FootageComboBox : public QComboBox
{
  Q_OBJECT
public:
  FootageComboBox(QWidget* parent = nullptr);

  virtual void showPopup() override;

  void SetRoot(const Folder *p);

  void SetOnlyShowReadyFootage(bool e);

  StreamPtr SelectedFootage();

public slots:
  void SetFootage(StreamPtr f);

signals:
  void FootageChanged(StreamPtr f);

private:
  void TraverseFolder(const Folder *f, QMenu* m);

  void UpdateText();

  static QString FootageToString(Stream* f);

  const Folder* root_;

  StreamPtr footage_;

  bool only_show_ready_footage_;
};

OLIVE_NAMESPACE_EXIT

#endif // FOOTAGECOMBOBOX_H
