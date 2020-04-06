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

#ifndef PROJECTPROPERTIESDIALOG_H
#define PROJECTPROPERTIESDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>

#include "project/project.h"

OLIVE_NAMESPACE_ENTER

class ProjectPropertiesDialog : public QDialog
{
  Q_OBJECT
public:
  ProjectPropertiesDialog(QWidget* parent);

public slots:
  virtual void accept() override;

private:
  bool VerifyOCIOConfig(const QString& fn);

  void ListPossibleInputSpaces(const QString &fn);

  Project* working_project_;

  QLineEdit* ocio_filename_;

  QComboBox* default_input_colorspace_;

private slots:
  void BrowseForOCIOConfig();

};

OLIVE_NAMESPACE_EXIT

#endif // PROJECTPROPERTIESDIALOG_H
