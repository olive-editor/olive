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

#include "preferencesqualitytab.h"

#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include "audio/sampleformat.h"
#include "render/colormanager.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

PreferencesQualityTab::PreferencesQualityTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QHBoxLayout* profile_layout = new QHBoxLayout();
  profile_layout->setMargin(0);

  profile_layout->addWidget(new QLabel(tr("Profile:")));

  QComboBox* profile_combobox = new QComboBox();
  profile_combobox->addItem(tr("Preview (Offline)"));
  profile_combobox->addItem(tr("Export (Online)"));
  profile_layout->addWidget(profile_combobox);

  layout->addLayout(profile_layout);

  quality_stack_ = new QStackedWidget();

  offline_group_ = new PreferencesQualityGroup(tr("Offline Quality"));
  offline_group_->bit_depth_combobox()->setCurrentIndex(PixelFormat::instance()->GetConfiguredFormatForMode(RenderMode::kOffline));
  offline_group_->ocio_method()->setCurrentIndex(ColorManager::GetOCIOMethodForMode(RenderMode::kOffline));
  quality_stack_->addWidget(offline_group_);

  online_group_ = new PreferencesQualityGroup(tr("Online Quality"));
  online_group_->bit_depth_combobox()->setCurrentIndex(PixelFormat::instance()->GetConfiguredFormatForMode(RenderMode::kOnline));
  online_group_->ocio_method()->setCurrentIndex(ColorManager::GetOCIOMethodForMode(RenderMode::kOnline));
  quality_stack_->addWidget(online_group_);

  layout->addWidget(quality_stack_);

  connect(profile_combobox, SIGNAL(currentIndexChanged(int)), quality_stack_, SLOT(setCurrentIndex(int)));
}

void PreferencesQualityTab::Accept()
{
  ColorManager::SetOCIOMethodForMode(RenderMode::kOffline, static_cast<ColorManager::OCIOMethod>(offline_group_->ocio_method()->currentIndex()));
  ColorManager::SetOCIOMethodForMode(RenderMode::kOnline, static_cast<ColorManager::OCIOMethod>(online_group_->ocio_method()->currentIndex()));
  PixelFormat::instance()->SetConfiguredFormatForMode(RenderMode::kOffline, static_cast<PixelFormat::Format>(offline_group_->bit_depth_combobox()->currentData().toInt()));
  PixelFormat::instance()->SetConfiguredFormatForMode(RenderMode::kOnline, static_cast<PixelFormat::Format>(online_group_->bit_depth_combobox()->currentData().toInt()));
}

PreferencesQualityGroup::PreferencesQualityGroup(const QString &title, QWidget *parent) :
  QGroupBox(title, parent)
{
  QVBoxLayout* quality_outer_layout = new QVBoxLayout(this);

  QGroupBox* video_group = new QGroupBox(tr("Video"));

  QGridLayout* video_layout = new QGridLayout(video_group);
  quality_outer_layout->addWidget(video_group);

  int row = 0;

  video_layout->addWidget(new QLabel(tr("Pixel Format:")), row, 0);

  bit_depth_combobox_ = new QComboBox();

  // Populate with bit depths
  for (int i=0;i<PixelFormat::PIX_FMT_COUNT;i++) {
    PixelFormat::Format pix_fmt = static_cast<PixelFormat::Format>(i);

    // We always render with an alpha channel internally
    if (PixelFormat::FormatHasAlphaChannel(pix_fmt)) {
      bit_depth_combobox_->addItem(PixelFormat::GetName(pix_fmt),
                                   i);
    }
  }

  video_layout->addWidget(bit_depth_combobox_, row, 1);

  row++;

  video_layout->addWidget(new QLabel(tr("OpenColorIO Method:")), row, 0);

  ocio_method_ = new QComboBox();
  ocio_method_->addItem(tr("Fast"));
  ocio_method_->addItem(tr("Accurate"));
  video_layout->addWidget(ocio_method_, row, 1);

  quality_outer_layout->addStretch();
}

QComboBox *PreferencesQualityGroup::bit_depth_combobox()
{
  return bit_depth_combobox_;
}

QComboBox *PreferencesQualityGroup::ocio_method()
{
  return ocio_method_;
}

OLIVE_NAMESPACE_EXIT
