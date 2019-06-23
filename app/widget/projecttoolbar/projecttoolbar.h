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

#ifndef PROJECTTOOLBAR_H
#define PROJECTTOOLBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class ProjectToolbar : public QWidget
{
  Q_OBJECT
public:
  ProjectToolbar(QWidget* parent);
protected:
  void changeEvent(QEvent *) override;
signals:
  void NewClicked();
  void OpenClicked();
  void SaveClicked();

  void UndoClicked();
  void RedoClicked();

  void SearchChanged(const QString&);

  void TreeViewClicked();
  void IconViewClicked();
  void ListViewClicked();
private:
  void Retranslate();

  QPushButton* new_button_;
  QPushButton* open_button_;
  QPushButton* save_button_;
  QPushButton* undo_button_;
  QPushButton* redo_button_;

  QLineEdit* search_field_;

  QPushButton* tree_button_;
  QPushButton* list_button_;
  QPushButton* icon_button_;
};

#endif // PROJECTTOOLBAR_H
