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

#include "preferencesdisktab.h"

#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

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

  disk_management_layout->addWidget(new QLabel(tr("Maximum Disk Cache:")), row, 0);

  maximum_cache_slider_ = new FloatSlider();
  maximum_cache_slider_->SetFormat(tr("%1 GB"));
  maximum_cache_slider_->SetMinimum(1.0);
  maximum_cache_slider_->SetValue(static_cast<double>(default_disk_cache_folder_->GetLimit()) / static_cast<double>(kBytesInGigabyte));
  disk_management_layout->addWidget(maximum_cache_slider_, row, 1);

  row++;

  clear_cache_btn_ = new QPushButton(tr("Clear Disk Cache"));
  connect(clear_cache_btn_, &QPushButton::clicked, this, &PreferencesDiskTab::ClearDiskCache);
  disk_management_layout->addWidget(clear_cache_btn_, row, 1);

  row++;

  clear_disk_cache_ = new QCheckBox(tr("Automatically clear disk cache on close"));
  clear_disk_cache_->setChecked(default_disk_cache_folder_->GetClearOnClose());
  disk_management_layout->addWidget(clear_disk_cache_, row, 1);

  QGroupBox* cache_behavior = new QGroupBox(tr("Cache Behavior"));
  outer_layout->addWidget(cache_behavior);
  QGridLayout* cache_behavior_layout = new QGridLayout(cache_behavior);

  row = 0;

  cache_behavior_layout->addWidget(new QLabel(tr("Cache Ahead:")), row, 0);

  cache_ahead_slider_ = new FloatSlider();
  cache_ahead_slider_->SetFormat(tr("%1 seconds"));
  cache_ahead_slider_->SetMinimum(0);
  cache_ahead_slider_->SetValue(Config::Current()["DiskCacheAhead"].value<rational>().toDouble());
  cache_behavior_layout->addWidget(cache_ahead_slider_, row, 1);

  cache_behavior_layout->addWidget(new QLabel(tr("Cache Behind:")), row, 2);

  cache_behind_slider_ = new FloatSlider();
  cache_behind_slider_->SetMinimum(0);
  cache_behind_slider_->SetFormat(tr("%1 seconds"));
  cache_behind_slider_->SetValue(Config::Current()["DiskCacheBehind"].value<rational>().toDouble());
  cache_behavior_layout->addWidget(cache_behind_slider_, row, 3);

  outer_layout->addStretch();
}

bool PreferencesDiskTab::Validate()
{
  if (disk_cache_location_->text() != default_disk_cache_folder_->GetPath()) {
    // Disk cache location is changing

    // Check if the user wants to move the cache here
    if (QMessageBox::question(this,
                              tr("Disk Cache"),
                              tr("You've chosen to change the default disk cache location. This "
                                 "will invalidate your current cache. Would you like to continue?"),
                              QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
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

  qint64 new_disk_cache_limit = qRound64(maximum_cache_slider_->GetValue() * kBytesInGigabyte);
  if (new_disk_cache_limit != default_disk_cache_folder_->GetLimit()) {
    default_disk_cache_folder_->SetLimit(new_disk_cache_limit);
  }

  if (default_disk_cache_folder_->GetClearOnClose() != clear_disk_cache_->isChecked()) {
    default_disk_cache_folder_->SetClearOnClose(clear_disk_cache_->isChecked());
  }

  Config::Current()["DiskCacheBehind"] = QVariant::fromValue(rational::fromDouble(cache_behind_slider_->GetValue()));
  Config::Current()["DiskCacheAhead"] = QVariant::fromValue(rational::fromDouble(cache_ahead_slider_->GetValue()));
}

void PreferencesDiskTab::ClearDiskCache()
{
  if (QMessageBox::question(this,
                            tr("Clear Disk Cache"),
                            tr("Are you sure you want to clear the disk cache in '%1'?").arg(Config::Current()["DiskCachePath"].toString()),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    clear_cache_btn_->setEnabled(false);

    if (DiskManager::instance()->ClearDiskCache(default_disk_cache_folder_->GetPath())) {
      clear_cache_btn_->setText(tr("Disk Cache Cleared"));
    } else {
      QMessageBox::information(this,
                               tr("Clear Disk Cache"),
                               tr("Disk cache failed to fully clear. You may have to delete the cache files manually."),
                               QMessageBox::Ok);
      clear_cache_btn_->setText(tr("Disk Cache Partially Cleared"));
    }
  }
}

OLIVE_NAMESPACE_EXIT
