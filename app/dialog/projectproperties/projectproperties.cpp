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

#include "projectproperties.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

#include "common/filefunctions.h"
#include "common/ocioutils.h"
#include "config/config.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "render/diskmanager.h"

namespace olive {

#define super QDialog

ProjectPropertiesDialog::ProjectPropertiesDialog(Project* p, QWidget *parent) :
  super(parent),
  working_project_(p),
  ocio_config_is_valid_(true)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  setWindowTitle(tr("Project Properties for '%1'").arg(working_project_->name()));

  QTabWidget* tabs = new QTabWidget;
  layout->addWidget(tabs);

  {
    // Color management group
    QWidget* color_group = new QWidget();

    QVBoxLayout* color_outer_layout = new QVBoxLayout(color_group);

    QGridLayout* color_layout = new QGridLayout();
    color_outer_layout->addLayout(color_layout);

    int row = 0;

    color_layout->addWidget(new QLabel(tr("OpenColorIO Configuration:")), row, 0);

    ocio_filename_ = new QLineEdit();
    ocio_filename_->setPlaceholderText(tr("(default)"));
    color_layout->addWidget(ocio_filename_, row, 1);

    row++;

    color_layout->addWidget(new QLabel(tr("Default Input Color Space:")), row, 0);

    default_input_colorspace_ = new QComboBox();
    color_layout->addWidget(default_input_colorspace_, row, 1, 1, 2);

    row++;

    QPushButton* browse_btn = new QPushButton(tr("Browse"));
    color_layout->addWidget(browse_btn, 0, 2);
    connect(browse_btn, &QPushButton::clicked, this, &ProjectPropertiesDialog::BrowseForOCIOConfig);

    ocio_filename_->setText(working_project_->color_manager()->GetConfigFilename());

    connect(ocio_filename_, &QLineEdit::textChanged, this, &ProjectPropertiesDialog::OCIOFilenameUpdated);
    OCIOFilenameUpdated();

    tabs->addTab(color_group, tr("Color Management"));

    color_outer_layout->addStretch();
  }

  {
    // Cache group
    QWidget* cache_group = new QWidget();

    QVBoxLayout* cache_layout = new QVBoxLayout(cache_group);

    QButtonGroup* disk_cache_btn_group = new QButtonGroup();

    // Create radio buttons and add to widget and button group
    disk_cache_radios_[ProjectSettingsNode::kCacheUseDefaultLocation] = new QRadioButton(tr("Use Default Location"));
    disk_cache_radios_[ProjectSettingsNode::kCacheStoreAlongsideProject] = new QRadioButton(tr("Store Alongside Project"));
    disk_cache_radios_[ProjectSettingsNode::kCacheCustomPath] = new QRadioButton(tr("Use Custom Location:"));
    for (int i=0; i<kDiskCacheRadioCount; i++) {
      disk_cache_btn_group->addButton(disk_cache_radios_[i]);
      cache_layout->addWidget(disk_cache_radios_[i]);
    }

    // Create custom cache path widget
    custom_cache_path_ = new PathWidget(working_project_->settings()->GetCustomCachePath(), this);
    custom_cache_path_->setEnabled(false);
    cache_layout->addWidget(custom_cache_path_);

    // Ensure custom cache path "enabled" is tied to the radio button being checked
    connect(disk_cache_radios_[ProjectSettingsNode::kCacheCustomPath], &QRadioButton::toggled, custom_cache_path_, &PathWidget::setEnabled);

    // Check the radio button that should currently be active
    disk_cache_radios_[working_project_->settings()->GetCacheSetting()]->setChecked(true);

    // Add disk cache settings button
    QPushButton* disk_cache_settings_btn = new QPushButton(tr("Disk Cache Settings"));
    connect(disk_cache_settings_btn, &QPushButton::clicked, this, &ProjectPropertiesDialog::OpenDiskCacheSettings);
    cache_layout->addWidget(disk_cache_settings_btn);

    tabs->addTab(cache_group, tr("Disk Cache"));
  }

  QDialogButtonBox* dialog_btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal);
  layout->addWidget(dialog_btns);
  connect(dialog_btns, &QDialogButtonBox::accepted, this, &ProjectPropertiesDialog::accept);
  connect(dialog_btns, &QDialogButtonBox::rejected, this, &ProjectPropertiesDialog::reject);
}

void ProjectPropertiesDialog::accept()
{
  if (!ocio_config_is_valid_) {
    QMessageBox mb(this);
    mb.setWindowModality(Qt::WindowModal);
    mb.setIcon(QMessageBox::Critical);
    mb.setWindowTitle(tr("OpenColorIO Config Error"));
    mb.setText(tr("Failed to set OpenColorIO configuration: %1").arg(ocio_config_error_));
    mb.addButton(QMessageBox::Ok);
    mb.exec();
    return;
  }

  if (disk_cache_radios_[ProjectSettingsNode::kCacheUseDefaultLocation]->isChecked()) {
    // Keep new cache path empty, which means default
  } else if (disk_cache_radios_[ProjectSettingsNode::kCacheStoreAlongsideProject]->isChecked()) {
    // Ensure alongside project path is valid
    if (!VerifyPathAndWarnIfBad(working_project_->get_cache_alongside_project_path())) {
      return;
    }
  } else {
    // Ensure custom path is valid
    if (!VerifyPathAndWarnIfBad(custom_cache_path_->text())) {
      return;
    }
  }

  if (custom_cache_path_->text() != working_project_->settings()->GetCustomCachePath()) {
    // Check if the user is okay with invalidating the current cache
    if (!DiskManager::ShowDiskCacheChangeConfirmationDialog(this)) {
      return;
    }

    working_project_->settings()->SetCustomCachePath(custom_cache_path_->text());

    emit DiskManager::instance()->InvalidateProject(working_project_);
  }

  // This should ripple changes throughout the graph/cache that the color config has changed, and
  // therefore should be done after the cache path is changed
  if (working_project_->color_manager()->GetConfigFilename() != ocio_filename_->text()) {
    working_project_->color_manager()->SetConfigFilename(ocio_filename_->text());
  }
  if (working_project_->color_manager()->GetDefaultInputColorSpace() != default_input_colorspace_->currentText()) {
    working_project_->color_manager()->SetDefaultInputColorSpace(default_input_colorspace_->currentText());
  }

  super::accept();
}

bool ProjectPropertiesDialog::VerifyPathAndWarnIfBad(const QString &path)
{
  if (!FileFunctions::DirectoryIsValid(path)) {
    QMessageBox mb(this);
    mb.setWindowModality(Qt::WindowModal);
    mb.setIcon(QMessageBox::Critical);
    mb.setWindowTitle(tr("Invalid path"));
    mb.setText(tr("The custom cache path is invalid. Please check it and try again."));
    mb.addButton(QMessageBox::Ok);
    mb.exec();
    return false;
  }

  return true;
}

void ProjectPropertiesDialog::BrowseForOCIOConfig()
{
  QString fn = QFileDialog::getOpenFileName(this, tr("Browse for OpenColorIO configuration"));
  if (!fn.isEmpty()) {
    ocio_filename_->setText(fn);
  }
}

void ProjectPropertiesDialog::OCIOFilenameUpdated()
{
  default_input_colorspace_->clear();

  try {
    OCIO::ConstConfigRcPtr c;

    if (ocio_filename_->text().isEmpty()) {
      c = ColorManager::GetDefaultConfig();
    } else {
      c = ColorManager::CreateConfigFromFile(ocio_filename_->text());
    }

    ocio_filename_->setStyleSheet(QString());
    ocio_config_is_valid_ = true;

    // List input color spaces
    QStringList input_cs = ColorManager::ListAvailableColorspaces(c);

    foreach (QString cs, input_cs) {
      default_input_colorspace_->addItem(cs);

      if (cs == working_project_->color_manager()->GetDefaultInputColorSpace()) {
        default_input_colorspace_->setCurrentIndex(default_input_colorspace_->count()-1);
      }
    }
  } catch (OCIO::Exception& e) {
    ocio_config_is_valid_ = false;
    ocio_filename_->setStyleSheet(QStringLiteral("QLineEdit {color: red;}"));
    ocio_config_error_ = e.what();
  }
}

void ProjectPropertiesDialog::OpenDiskCacheSettings()
{
  if (disk_cache_radios_[ProjectSettingsNode::kCacheUseDefaultLocation]->isChecked()) {
    DiskManager::instance()->ShowDiskCacheSettingsDialog(DiskManager::instance()->GetDefaultCacheFolder(), this);
  } else if (disk_cache_radios_[ProjectSettingsNode::kCacheStoreAlongsideProject]->isChecked()) {
    DiskManager::instance()->ShowDiskCacheSettingsDialog(working_project_->get_cache_alongside_project_path(), this);
  } else {
    DiskManager::instance()->ShowDiskCacheSettingsDialog(custom_cache_path_->text(), this);
  }
}

}
