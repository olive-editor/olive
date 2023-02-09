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

#ifndef PROJECTPROPERTIESDIALOG_H
#define PROJECTPROPERTIESDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QRadioButton>

#include "node/project/project.h"
#include "widget/path/pathwidget.h"

namespace olive {

class ProjectPropertiesDialog : public QDialog
{
  Q_OBJECT
public:
  ProjectPropertiesDialog(Project *p, QWidget* parent);

public slots:
  virtual void accept() override;

private:
  bool VerifyPathAndWarnIfBad(const QString &path);

  Project* working_project_;

  QLineEdit* ocio_filename_;

  QComboBox* default_input_colorspace_;

  bool ocio_config_is_valid_;

  QString ocio_config_error_;

  PathWidget* custom_cache_path_;

  static const int kDiskCacheRadioCount = 3;
  QRadioButton *disk_cache_radios_[kDiskCacheRadioCount];

private slots:
  void BrowseForOCIOConfig();

  void OCIOFilenameUpdated();

  void OpenDiskCacheSettings();

};

}

#endif // PROJECTPROPERTIESDIALOG_H
