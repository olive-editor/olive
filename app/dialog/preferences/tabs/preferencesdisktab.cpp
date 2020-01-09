#include "preferencesdisktab.h"

#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

PreferencesDiskTab::PreferencesDiskTab()
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGridLayout* layout = new QGridLayout();
  outer_layout->addLayout(layout);

  int row = 0;

  layout->addWidget(new QLabel(tr("Disk Cache Location:")), row, 0);

  disk_cache_location_ = new QLineEdit();
  disk_cache_location_->setText(Config::Current()["DiskCachePath"].toString());
  connect(disk_cache_location_, &QLineEdit::textChanged, this, &PreferencesDiskTab::DiskCacheLineEditChanged);
  layout->addWidget(disk_cache_location_, row, 1);

  QPushButton* browse_btn = new QPushButton(tr("Browse"));
  connect(browse_btn, &QPushButton::clicked, this, &PreferencesDiskTab::BrowseDiskCachePath);
  layout->addWidget(browse_btn, row, 2);

  row++;

  layout->addWidget(new QLabel(tr("Maximum Disk Cache:")), row, 0);

  maximum_cache_slider_ = new FloatSlider();
  maximum_cache_slider_->SetSuffix(QStringLiteral(" GB"));
  maximum_cache_slider_->SetMinimum(1.0);
  maximum_cache_slider_->SetValue(Config::Current()["DiskCacheSize"].toDouble());
  layout->addWidget(maximum_cache_slider_, row, 1, 1, 2);

  row++;

  QPushButton* clear_cache_btn = new QPushButton(tr("Clear Disk Cache"));
  connect(clear_cache_btn, &QPushButton::clicked, this, &PreferencesDiskTab::ClearDiskCache);
  layout->addWidget(clear_cache_btn, row, 1, 1, 2);

  row++;

  clear_disk_cache_ = new QCheckBox(tr("Automatically clear disk cache on close"));
  clear_disk_cache_->setChecked(Config::Current()["ClearDiskCacheOnClose"].toBool());
  layout->addWidget(clear_disk_cache_, row, 1, 1, 2);

  outer_layout->addStretch();
}

void PreferencesDiskTab::Accept()
{
  Config::Current()["DiskCachePath"] = disk_cache_location_->text();
  Config::Current()["DiskCacheSize"] = maximum_cache_slider_->GetValue();
  Config::Current()["ClearDiskCacheOnClose"] = clear_disk_cache_->isChecked();
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
  qDebug() << "Clear" << Config::Current()["DiskCachePath"];
}
