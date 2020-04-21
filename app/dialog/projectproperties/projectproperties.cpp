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

#include "projectproperties.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "config/config.h"
#include "core.h"
#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

ProjectPropertiesDialog::ProjectPropertiesDialog(Project* p, QWidget *parent) :
  QDialog(parent),
  working_project_(p)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  setWindowTitle(tr("Project Properties for '%1'").arg(working_project_->name()));

  QGroupBox* color_group = new QGroupBox();
  color_group->setTitle(tr("Color Management"));

  QGridLayout* color_layout = new QGridLayout(color_group);

  int row = 0;

  color_layout->addWidget(new QLabel(tr("OpenColorIO Configuration:")), row, 0);

  ocio_filename_ = new QLineEdit();
  color_layout->addWidget(ocio_filename_, row, 1);

  row++;

  color_layout->addWidget(new QLabel(tr("Default Input Color Space:")), row, 0);

  default_input_colorspace_ = new QComboBox();
  color_layout->addWidget(default_input_colorspace_, row, 1, 1, 2);

  row++;

  QPushButton* browse_btn = new QPushButton(tr("Browse"));
  color_layout->addWidget(browse_btn, 0, 2);
  connect(browse_btn, SIGNAL(clicked(bool)), this, SLOT(BrowseForOCIOConfig()));

  layout->addWidget(color_group);

  QDialogButtonBox* dialog_btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
  layout->addWidget(dialog_btns);
  connect(dialog_btns, SIGNAL(accepted()), this, SLOT(accept()));
  connect(dialog_btns, SIGNAL(rejected()), this, SLOT(reject()));

  if (working_project_ == nullptr) {
    QMessageBox::critical(this,
                          tr("No Active Project"),
                          tr("No project is currently open to set the properties for"),
                          QMessageBox::Ok);
    reject();
    return;
  }

  ocio_filename_->setText(working_project_->color_manager()->GetConfigFilename());
  ListPossibleInputSpaces(working_project_->color_manager()->GetConfigFilename());
}

void ProjectPropertiesDialog::accept()
{
  try {
    // This should ripple changes throughout the program that the color config has changed, therefore must be done last
    working_project_->color_manager()->SetConfigAndDefaultInput(ocio_filename_->text(), default_input_colorspace_->currentText());

    QDialog::accept();
  } catch (OCIO::Exception& e) {
    QMessageBox::critical(this,
                          tr("OpenColorIO Config Error"),
                          tr("Failed to set OpenColorIO configuration: %1").arg(e.what()),
                          QMessageBox::Ok);
  }
}

bool ProjectPropertiesDialog::VerifyOCIOConfig(const QString &fn)
{
  try {
    OCIO::Config::CreateFromFile(fn.toUtf8());

    return true;
  } catch (OCIO::Exception& e) {
    QMessageBox::critical(this,
                          tr("OpenColorIO Config Error"),
                          tr("Failed to set OpenColorIO configuration: %1").arg(e.what()),
                          QMessageBox::Ok);

    return false;
  }
}

void ProjectPropertiesDialog::ListPossibleInputSpaces(const QString& fn)
{
  try {
    default_input_colorspace_->clear();

    QStringList input_cs = ColorManager::ListAvailableInputColorspaces(OCIO::Config::CreateFromFile(fn.toUtf8()));

    foreach (QString cs, input_cs) {
      default_input_colorspace_->addItem(cs);

      if (cs == working_project_->color_manager()->GetDefaultInputColorSpace()) {
        default_input_colorspace_->setCurrentIndex(default_input_colorspace_->count()-1);
      }
    }
  } catch (OCIO::Exception&) {
  }
}

void ProjectPropertiesDialog::BrowseForOCIOConfig()
{
  QString fn = QFileDialog::getOpenFileName(this, tr("Browse for OpenColorIO configuration"));
  if (!fn.isEmpty()) {
    if (VerifyOCIOConfig(fn)) {
      ocio_filename_->setText(fn);

      ListPossibleInputSpaces(fn);
    }
  }
}

OLIVE_NAMESPACE_EXIT
