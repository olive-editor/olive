#include "preferencesdisktab.h"

#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

#include "render/diskmanager.h"

PreferencesDiskTab::PreferencesDiskTab()
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGroupBox* disk_management_group = new QGroupBox(tr("Disk Management"));
  outer_layout->addWidget(disk_management_group);

  QGridLayout* disk_management_layout = new QGridLayout(disk_management_group);

  int row = 0;

  disk_management_layout->addWidget(new QLabel(tr("Disk Cache Location:")), row, 0);

  disk_cache_location_ = new QLineEdit();
  disk_cache_location_->setText(Config::Current()["DiskCachePath"].toString());
  connect(disk_cache_location_, &QLineEdit::textChanged, this, &PreferencesDiskTab::DiskCacheLineEditChanged);
  disk_management_layout->addWidget(disk_cache_location_, row, 1);

  QPushButton* browse_btn = new QPushButton(tr("Browse"));
  connect(browse_btn, &QPushButton::clicked, this, &PreferencesDiskTab::BrowseDiskCachePath);
  disk_management_layout->addWidget(browse_btn, row, 2);

  row++;

  disk_management_layout->addWidget(new QLabel(tr("Maximum Disk Cache:")), row, 0);

  maximum_cache_slider_ = new FloatSlider();
  maximum_cache_slider_->SetSuffix(QStringLiteral(" GB"));
  maximum_cache_slider_->SetMinimum(1.0);
  maximum_cache_slider_->SetValue(Config::Current()["DiskCacheSize"].toDouble());
  disk_management_layout->addWidget(maximum_cache_slider_, row, 1, 1, 2);

  row++;

  QPushButton* clear_cache_btn = new QPushButton(tr("Clear Disk Cache"));
  connect(clear_cache_btn, &QPushButton::clicked, this, &PreferencesDiskTab::ClearDiskCache);
  disk_management_layout->addWidget(clear_cache_btn, row, 1, 1, 2);

  row++;

  clear_disk_cache_ = new QCheckBox(tr("Automatically clear disk cache on close"));
  clear_disk_cache_->setChecked(Config::Current()["ClearDiskCacheOnClose"].toBool());
  disk_management_layout->addWidget(clear_disk_cache_, row, 1, 1, 2);

  QGroupBox* cache_behavior = new QGroupBox(tr("Cache Behavior"));
  outer_layout->addWidget(cache_behavior);
  QGridLayout* cache_behavior_layout = new QGridLayout(cache_behavior);

  row = 0;

  cache_behavior_layout->addWidget(new QLabel(tr("Cache Ahead:")), row, 0);

  cache_ahead_slider_ = new FloatSlider();
  cache_ahead_slider_->SetSuffix(QStringLiteral(" seconds"));
  cache_ahead_slider_->SetValue(Config::Current()["DiskCacheAhead"].value<rational>().toDouble());
  cache_behavior_layout->addWidget(cache_ahead_slider_, row, 1);

  cache_behavior_layout->addWidget(new QLabel(tr("Cache Behind:")), row, 2);

  cache_behind_slider_ = new FloatSlider();
  cache_behind_slider_->SetSuffix(QStringLiteral(" seconds"));
  cache_behind_slider_->SetValue(Config::Current()["DiskCacheBehind"].value<rational>().toDouble());
  cache_behavior_layout->addWidget(cache_behind_slider_, row, 3);

  outer_layout->addStretch();
}

void PreferencesDiskTab::Accept()
{
  Config::Current()["DiskCachePath"] = disk_cache_location_->text();
  Config::Current()["DiskCacheSize"] = maximum_cache_slider_->GetValue();
  Config::Current()["ClearDiskCacheOnClose"] = clear_disk_cache_->isChecked();
  Config::Current()["DiskCacheBehind"] = QVariant::fromValue(rational::fromDouble(cache_behind_slider_->GetValue()));
  Config::Current()["DiskCacheAhead"] = QVariant::fromValue(rational::fromDouble(cache_ahead_slider_->GetValue()));
}

void PreferencesDiskTab::DiskCacheLineEditChanged()
{
  QString entered_dir = disk_cache_location_->text();

  if (!entered_dir.isEmpty() && !QDir(entered_dir).exists()) {
    disk_cache_location_->setStyleSheet(QStringLiteral("color: red;"));
  } else {
    disk_cache_location_->setStyleSheet(QString());
  }
}

void PreferencesDiskTab::BrowseDiskCachePath()
{
  QString dir = QFileDialog::getExistingDirectory(this);

  if (!dir.isEmpty()) {
    disk_cache_location_->setText(dir);
  }
}

void PreferencesDiskTab::ClearDiskCache()
{
  if (QMessageBox::question(this,
                            tr("Clear Disk Cache"),
                            tr("Are you sure you want to clear the disk cache in '%1'?").arg(Config::Current()["DiskCachePath"].toString()),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    if (DiskManager::instance()->ClearDiskCache(false)) {
      QMessageBox::information(this,
                               tr("Clear Disk Cache"),
                               tr("Disk cache cleared successfully"),
                               QMessageBox::Ok);
    } else {
      QMessageBox::information(this,
                               tr("Clear Disk Cache"),
                               tr("Disk cache failed to fully clear. You may have to delete the cache files manually."),
                               QMessageBox::Ok);
    }
  }
}
