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

#include "diskcachedialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>

#include "config/config.h"

OLIVE_NAMESPACE_ENTER

DiskCacheDialog::DiskCacheDialog(DiskCacheFolder *folder, QWidget* parent) :
  QDialog(parent),
  folder_(folder)
{
  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Disk Cache: %1").arg(folder->GetPath())), row, 0, 1, 2);
  setWindowTitle(tr("Disk Cache Settings"));

  row++;

  layout->addWidget(new QLabel(tr("Maximum Disk Cache:")), row, 0);

  maximum_cache_slider_ = new FloatSlider();
  maximum_cache_slider_->SetFormat(tr("%1 GB"));
  maximum_cache_slider_->SetMinimum(1.0);
  maximum_cache_slider_->SetValue(static_cast<double>(folder->GetLimit()) / static_cast<double>(kBytesInGigabyte));
  layout->addWidget(maximum_cache_slider_, row, 1);

  row++;

  clear_cache_btn_ = new QPushButton(tr("Clear Disk Cache"));
  connect(clear_cache_btn_, &QPushButton::clicked, this, &DiskCacheDialog::ClearDiskCache);
  layout->addWidget(clear_cache_btn_, row, 1);

  row++;

  clear_disk_cache_ = new QCheckBox(tr("Automatically clear disk cache on close"));
  clear_disk_cache_->setChecked(folder->GetClearOnClose());
  layout->addWidget(clear_disk_cache_, row, 1);

  row++;

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &DiskCacheDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &DiskCacheDialog::reject);
  layout->addWidget(buttons, row, 0, 1, 2);
}

void DiskCacheDialog::accept()
{
  qint64 new_disk_cache_limit = qRound64(maximum_cache_slider_->GetValue() * kBytesInGigabyte);
  if (new_disk_cache_limit != folder_->GetLimit()) {
    folder_->SetLimit(new_disk_cache_limit);
  }

  if (folder_->GetClearOnClose() != clear_disk_cache_->isChecked()) {
    folder_->SetClearOnClose(clear_disk_cache_->isChecked());
  }

  QDialog::accept();
}

void DiskCacheDialog::ClearDiskCache()
{
  if (QMessageBox::question(this,
                            tr("Clear Disk Cache"),
                            tr("Are you sure you want to clear the disk cache in '%1'?").arg(Config::Current()["DiskCachePath"].toString()),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    clear_cache_btn_->setEnabled(false);

    if (DiskManager::instance()->ClearDiskCache(folder_->GetPath())) {
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
