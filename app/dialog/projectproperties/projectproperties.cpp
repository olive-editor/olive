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
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "render/colormanager.h"

ProjectPropertiesDialog::ProjectPropertiesDialog(QWidget *parent) :
  QDialog(parent)
{
  QGridLayout* layout = new QGridLayout(this);

  setWindowTitle(tr("Project Properties"));

  layout->addWidget(new QLabel(tr("OpenColorIO Configuration:")), 0, 0);

  ocio_filename_ = new QLineEdit();
  layout->addWidget(ocio_filename_, 0, 1);

  QPushButton* browse_btn = new QPushButton(tr("Browse"));
  layout->addWidget(browse_btn, 0, 2);
  connect(browse_btn, SIGNAL(clicked(bool)), this, SLOT(BrowseForOCIOConfig()));

  QDialogButtonBox* dialog_btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
  layout->addWidget(dialog_btns, 1, 0, 1, 3);
  connect(dialog_btns, SIGNAL(accepted()), this, SLOT(accept()));
  connect(dialog_btns, SIGNAL(rejected()), this, SLOT(reject()));
}

void ProjectPropertiesDialog::accept()
{
  try {
    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile(ocio_filename_->text().toUtf8());

    ColorManager::instance()->SetConfig(config);

    QDialog::accept();
  } catch (OCIO::Exception& e) {
    QMessageBox::critical(this,
                          tr("OpenColorIO Config Error"),
                          tr("Failed to set OpenColorIO configuration: %1").arg(e.what()),
                          QMessageBox::Ok);
  }
}

void ProjectPropertiesDialog::BrowseForOCIOConfig()
{
  QString fn = QFileDialog::getOpenFileName(this, tr("Browse for OpenColorIO configuration"));
  if (!fn.isEmpty()) {
    ocio_filename_->setText(fn);
  }
}
