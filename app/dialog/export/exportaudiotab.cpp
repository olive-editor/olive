/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

namespace olive {

const int ExportAudioTab::kDefaultBitRate = 320;

ExportAudioTab::ExportAudioTab(QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGridLayout* layout = new QGridLayout();
  outer_layout->addLayout(layout);

  int row = 0;

  layout->addWidget(new QLabel(tr("Codec:")), row, 0);

  codec_combobox_ = new QComboBox();
  connect(codec_combobox_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportAudioTab::UpdateSampleFormats);
  connect(codec_combobox_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportAudioTab::UpdateBitRateEnabled);
  layout->addWidget(codec_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);

  sample_rate_combobox_ = new SampleRateComboBox();
  layout->addWidget(sample_rate_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Channel Layout:")), row, 0);

  channel_layout_combobox_ = new ChannelLayoutComboBox();
  layout->addWidget(channel_layout_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Format:")), row, 0);

  sample_format_combobox_ = new SampleFormatComboBox();
  layout->addWidget(sample_format_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Bit Rate:")), row, 0);

  bit_rate_slider_ = new IntegerSlider();
  bit_rate_slider_->SetMinimum(32);
  bit_rate_slider_->SetMaximum(320);
  bit_rate_slider_->SetValue(kDefaultBitRate);
  bit_rate_slider_->SetFormat(tr("%1 kbps"));
  layout->addWidget(bit_rate_slider_, row, 1);

  outer_layout->addStretch();
}

int ExportAudioTab::SetFormat(ExportFormat::Format format)
{
  QList<ExportCodec::Codec> acodecs = ExportFormat::GetAudioCodecs(format);
  setEnabled(!acodecs.isEmpty());
  codec_combobox_->blockSignals(true);
  codec_combobox_->clear();
  foreach (ExportCodec::Codec acodec, acodecs) {
    codec_combobox_->addItem(ExportCodec::GetCodecName(acodec), acodec);
  }
  codec_combobox_->blockSignals(false);
  fmt_ = format;

  UpdateSampleFormats();
  UpdateBitRateEnabled();

  return acodecs.size();
}

void ExportAudioTab::UpdateSampleFormats()
{
  auto fmts = ExportFormat::GetSampleFormatsForCodec(fmt_, GetCodec());
  sample_format_combobox_->SetAvailableFormats(fmts);
}

void ExportAudioTab::UpdateBitRateEnabled()
{
  bool uses_bitrate = !ExportCodec::IsCodecLossless(GetCodec());
  bit_rate_slider_->setEnabled(uses_bitrate);

  if (!uses_bitrate) {
    bit_rate_slider_->SetTristate();
  } else {
    bit_rate_slider_->SetValue(kDefaultBitRate  );
  }
}

}
