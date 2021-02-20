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

#ifndef FILEFIELD_H
#define FILEFIELD_H

#include <QLineEdit>
#include <QPushButton>

namespace olive {

class FileField : public QWidget
{
  Q_OBJECT
public:
  FileField(QWidget* parent = nullptr);

  QString GetFilename() const
  {
    return line_edit_->text();
  }

  void SetFilename(const QString& s)
  {
    line_edit_->setText(s);
  }

  void SetPlaceholder(const QString& s)
  {
    line_edit_->setPlaceholderText(s);
  }

  void SetDirectoryMode(bool e)
  {
    directory_mode_ = e;
  }

signals:
  void FilenameChanged(const QString& filename);

private:
  QLineEdit* line_edit_;

  QPushButton* browse_btn_;

  bool directory_mode_;

private slots:
  void BrowseBtnClicked();

  void LineEditChanged(const QString &text);

};

}

#endif // FILEFIELD_H
