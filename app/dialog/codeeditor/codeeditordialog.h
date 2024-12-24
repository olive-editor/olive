/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#ifndef CODEEDITORDIALOG_H
#define CODEEDITORDIALOG_H

#include <QDialog>
#include "dialog/codeeditor/editor.h"

namespace olive {

class SearchTextBar;


/// @brief Wrapper QDialog window for the code editor
class CodeEditorDialog : public QDialog
{
  Q_OBJECT
public:
  CodeEditorDialog(const QString &start, QWidget* parent = nullptr);

  QString text() const
  {
    return text_edit_->toPlainText();
  }

private:
  CodeEditor* text_edit_;
  SearchTextBar * search_bar_;

private slots:
  void OnActionAddInputTexture();
  void OnActionAddInputColor();
  void OnActionAddInputFloat();
  void OnActionAddInputInt();
  void OnActionAddInputBoolean();
  void OnActionAddInputSelection();
  void OnActionAddInputPoint();
  void OnFindRequest();
};

}  // namespace olive

#endif // CODEEDITORDIALOG_H
