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

#include "preferencesdisktab.h"

#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

#include "common/filefunctions.h"

namespace olive {

PreferencesDiskTab::PreferencesDiskTab()
{
  // Get default disk cache folder
  default_disk_cache_folder_ = DiskManager::instance()->GetDefaultCacheFolder();

  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGroupBox* disk_management_group = new QGroupBox(tr("Disk Management"));
  outer_layout->addWidget(disk_management_group);

  QGridLayout* disk_management_layout = new QGridLayout(disk_management_group);

  int row = 0;

  disk_management_layout->addWidget(new QLabel(tr("Disk Cache Location:")), row, 0);

  disk_cache_location_ = new PathWidget(default_disk_cache_folder_->GetPath());
  disk_management_layout->addWidget(disk_cache_location_, row, 1);

  row++;

  QPushButton* disk_cache_settings_btn = new QPushButton(tr("Disk Cache Settings"));
  connect(disk_cache_settings_btn, &QPushButton::clicked, this, [this](){
    DiskManager::instance()->ShowDiskCacheSettingsDialog(disk_cache_location_->text(), this);
  });
  disk_management_layout->addWidget(disk_cache_settings_btn, row, 1);

  row++;

  QGroupBox* cache_behavior = new QGroupBox(tr("Cache Behavior"));
  outer_layout->addWidget(cache_behavior);
  QGridLayout* cache_behavior_layout = new QGridLayout(cache_behavior);

  row = 0;

  cache_behavior_layout->addWidget(new QLabel(tr("Cache Ahead:")), row, 0);

  cache_ahead_slider_ = new FloatSlider();
  cache_ahead_slider_->SetFormat(tr("%1 second(s)"));
  cache_ahead_slider_->SetMinimum(0);
  cache_ahead_slider_->SetValue(Config::Current()["DiskCacheAhead"].value<rational>().toDouble());
  cache_behavior_layout->addWidget(cache_ahead_slider_, row, 1);

  cache_behavior_layout->addWidget(new QLabel(tr("Cache Behind:")), row, 2);

  cache_behind_slider_ = new FloatSlider();
  cache_behind_slider_->SetMinimum(0);
  cache_behind_slider_->SetFormat(tr("%1 second(s)"));
  cache_behind_slider_->SetValue(Config::Current()["DiskCacheBehind"].value<rational>().toDouble());
  cache_behavior_layout->addWidget(cache_behind_slider_, row, 3);

  outer_layout->addStretch();
}

bool PreferencesDiskTab::Validate()
{
  if (disk_cache_location_->text() != default_disk_cache_folder_->GetPath()) {
    // Disk cache location is changing

    // Check if the user is okay with invalidating the current cache
    if (!DiskManager::ShowDiskCacheChangeConfirmationDialog(this)) {
      return false;
    }

    // Check validity of the new path
    if (!FileFunctions::DirectoryIsValid(disk_cache_location_->text(), true)) {
      QMessageBox::critical(this,
                            tr("Disk Cache"),
                            tr("Failed to set disk cache location. Access was denied."));
      return false;
    }
  }

  return true;
}

void PreferencesDiskTab::Accept()
{
  if (disk_cache_location_->text() != default_disk_cache_folder_->GetPath()) {
    default_disk_cache_folder_->SetPath(disk_cache_location_->text());
  }

  Config::Current()["DiskCacheBehind"] = QVariant::fromValue(rational::fromDouble(cache_behind_slider_->GetValue()));
  Config::Current()["DiskCacheAhead"] = QVariant::fromValue(rational::fromDouble(cache_ahead_slider_->GetValue()));
}

}
