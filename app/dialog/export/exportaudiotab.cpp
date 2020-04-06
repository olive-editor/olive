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

#include "exportaudiotab.h"

#include <QGridLayout>
#include <QLabel>

#include "core.h"

OLIVE_NAMESPACE_ENTER

ExportAudioTab::ExportAudioTab(QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGridLayout* layout = new QGridLayout();
  outer_layout->addLayout(layout);

  int row = 0;

  layout->addWidget(new QLabel(tr("Codec:")), row, 0);

  codec_combobox_ = new QComboBox();
  layout->addWidget(codec_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);

  sample_rate_combobox_ = new QComboBox();
  sample_rates_ = Core::SupportedSampleRates();
  foreach (const int& sr, sample_rates_) {
    sample_rate_combobox_->addItem(Core::SampleRateToString(sr), sr);
  }
  layout->addWidget(sample_rate_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Channel Layout:")), row, 0);

  channel_layout_combobox_ = new QComboBox();
  channel_layouts_ = Core::SupportedChannelLayouts();
  foreach (const uint64_t& ch_layout, channel_layouts_) {
    channel_layout_combobox_->addItem(Core::ChannelLayoutToString(ch_layout), QVariant::fromValue(ch_layout));
  }
  layout->addWidget(channel_layout_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Format:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

  outer_layout->addStretch();
}

QComboBox *ExportAudioTab::codec_combobox() const
{
  return codec_combobox_;
}

QComboBox *ExportAudioTab::sample_rate_combobox() const
{
  return sample_rate_combobox_;
}

QComboBox *ExportAudioTab::channel_layout_combobox() const
{
  return channel_layout_combobox_;
}

void ExportAudioTab::set_sample_rate(int rate)
{
  sample_rate_combobox_->setCurrentIndex(sample_rates_.indexOf(rate));
}

void ExportAudioTab::set_channel_layout(uint64_t layout)
{
  channel_layout_combobox_->setCurrentIndex(channel_layouts_.indexOf(layout));
}

OLIVE_NAMESPACE_EXIT
