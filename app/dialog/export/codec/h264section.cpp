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

#include "h264section.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>

#include "common/qtutils.h"
#include "widget/slider/integerslider.h"

namespace olive {

H264Section::H264Section(QWidget *parent) :
  H264Section(H264CRFSection::kDefaultH264CRF, parent)
{
}

H264Section::H264Section(int default_crf, QWidget *parent) :
  CodecSection(parent)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  int row = 0;
  layout->addWidget(new QLabel(tr("Encode Speed:")), row, 0);


  preset_combobox_ = new QComboBox();
  preset_combobox_->setToolTip(tr("This setting allows you to tweak the ratio of export speed to compression quality. \n\n"
    "If using Constant Rate Factor, slower speeds will result in smaller file sizes for the same quality. \n\n"
    "If using Target Bit Rate or Target File Size, slower speeds will result in higher quality for the same bitrate/filesize. \n\n"
    "This setting is equivalent to the `preset` setting in libx264."));

  preset_combobox_->addItem(tr("Ultra Fast"));
  preset_combobox_->addItem(tr("Super Fast"));
  preset_combobox_->addItem(tr("Very Fast"));
  preset_combobox_->addItem(tr("Faster"));
  preset_combobox_->addItem(tr("Fast"));
  preset_combobox_->addItem(tr("Medium"));
  preset_combobox_->addItem(tr("Slow"));
  preset_combobox_->addItem(tr("Slower"));
  preset_combobox_->addItem(tr("Very Slow"));

  //Default to "medium"
  preset_combobox_->setCurrentIndex(5);

  layout->addWidget(preset_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Compression Method:")), row, 0);

  QComboBox* compression_box = new QComboBox();

  // These items must correspond to the CompressionMethod enum
  compression_box->addItem(tr("Constant Rate Factor"));
  compression_box->addItem(tr("Target Bit Rate"));
  compression_box->addItem(tr("Target File Size"));

  layout->addWidget(compression_box, row, 1);

  row++;

  compression_method_stack_ = new QStackedWidget();
  layout->addWidget(compression_method_stack_, row, 0, 1, 2);

  crf_section_ = new H264CRFSection(default_crf);
  compression_method_stack_->addWidget(crf_section_);

  bitrate_section_ = new H264BitRateSection();
  compression_method_stack_->addWidget(bitrate_section_);

  filesize_section_ = new H264FileSizeSection();
  compression_method_stack_->addWidget(filesize_section_);

  connect(compression_box,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          compression_method_stack_,
          &QStackedWidget::setCurrentIndex);
}

void H264Section::AddOpts(EncodingParams *params)
{
  // FIXME: Implement two-pass

  CompressionMethod method = static_cast<CompressionMethod>(compression_method_stack_->currentIndex());

  // This option is not used by the encoder (nor is anything with the ove_ prefix), it's to help us
  // identify which option was chosen when params are restored
  params->set_video_option(QStringLiteral("ove_compressionmethod"), QString::number(method));

  if (method == kConstantRateFactor) {

    // Simply set CRF value
    params->set_video_option(QStringLiteral("crf"), QString::number(crf_section_->GetValue()));

  } else {

    int64_t target_rate, max_rate, min_rate;

    if (method == kTargetBitRate) {
      // Use user-supplied values for the bit rate
      target_rate = bitrate_section_->GetTargetBitRate();
      min_rate = 0;
      max_rate = bitrate_section_->GetMaximumBitRate();
    } else {
      // Calculate the bit rate from the file size divided by the sequence length in seconds (bits per second)
      int64_t target_fs = filesize_section_->GetFileSize();
      target_rate = qRound64(static_cast<double>(target_fs) / params->GetExportLength().toDouble());
      min_rate = target_rate;
      max_rate = target_rate;

      params->set_video_option(QStringLiteral("ove_targetfilesize"), QString::number(target_fs));
    }

    // Disable CRF encoding
    params->set_video_option(QStringLiteral("crf"), QStringLiteral("-1"));

    params->set_video_bit_rate(target_rate);
    params->set_video_min_bit_rate(min_rate);
    params->set_video_max_bit_rate(max_rate);
    params->set_video_buffer_size(2000000);

  }

  params->set_video_option(QStringLiteral("preset"), QString::number(preset_combobox_->currentIndex()));
}

void H264Section::SetOpts(const EncodingParams *p)
{
  CompressionMethod method = static_cast<CompressionMethod>(p->video_option(QStringLiteral("ove_compressionmethod")).toInt());

  compression_method_stack_->setCurrentIndex(method);

  if (method == kConstantRateFactor) {
    crf_section_->SetValue(p->video_option(QStringLiteral("crf")).toInt());
  } else {
    int64_t target_rate = p->video_bit_rate();
    int64_t max_rate = p->video_max_bit_rate();

    if (method == kTargetBitRate) {
      // Use user-supplied values for the bit rate
      bitrate_section_->SetTargetBitRate(target_rate);
      bitrate_section_->SetMaximumBitRate(max_rate);
    } else {
      // Calculate the bit rate from the file size divided by the sequence length in seconds (bits per second)
      filesize_section_->SetFileSize(p->video_option(QStringLiteral("ove_targetfilesize")).toLongLong());
    }
  }
}

H264CRFSection::H264CRFSection(int default_crf, QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  crf_slider_ = new QSlider(Qt::Horizontal);
  crf_slider_->setMinimum(kMinimumCRF);
  crf_slider_->setMaximum(kMaximumCRF);
  crf_slider_->setValue(default_crf);
  layout->addWidget(crf_slider_);

  IntegerSlider* crf_input = new IntegerSlider();
  crf_input->setMaximumWidth(QtUtils::QFontMetricsWidth(crf_input->fontMetrics(), QStringLiteral("HHHH")));
  crf_input->SetMinimum(kMinimumCRF);
  crf_input->SetMaximum(kMaximumCRF);
  crf_input->SetValue(default_crf);
  crf_input->SetDefaultValue(default_crf);
  layout->addWidget(crf_input);

  connect(crf_slider_, &QSlider::valueChanged, crf_input, &IntegerSlider::SetValue);
  connect(crf_input, &IntegerSlider::ValueChanged, crf_slider_, &QSlider::setValue);
}

int H264CRFSection::GetValue() const
{
  return crf_slider_->value();
}

void H264CRFSection::SetValue(int c)
{
  crf_slider_->setValue(c);
}

H264BitRateSection::H264BitRateSection(QWidget *parent) :
  QWidget(parent)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  int row = 0;

  layout->addWidget(new QLabel(tr("Target Bit Rate (Mbps):")), row, 0);

  target_rate_ = new FloatSlider();
  target_rate_->SetMinimum(0);
  layout->addWidget(target_rate_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Maximum Bit Rate (Mbps):")), row, 0);

  max_rate_ = new FloatSlider();
  max_rate_->SetMinimum(0);
  layout->addWidget(max_rate_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Two-Pass")), row, 0);

  QCheckBox* two_pass_box = new QCheckBox();
  layout->addWidget(two_pass_box, row, 1);

  // Bit rate defaults
  target_rate_->SetValue(16.0);
  max_rate_->SetValue(32.0);
}

int64_t H264BitRateSection::GetTargetBitRate() const
{
  return qRound64(target_rate_->GetValue() * 1000000.0);
}

void H264BitRateSection::SetTargetBitRate(int64_t b)
{
  target_rate_->SetValue(double(b) * 0.000001);
}

int64_t H264BitRateSection::GetMaximumBitRate() const
{
  return qRound64(max_rate_->GetValue() * 1000000.0);
}

void H264BitRateSection::SetMaximumBitRate(int64_t b)
{
  max_rate_->SetValue(double(b) * 0.000001);
}

H264FileSizeSection::H264FileSizeSection(QWidget *parent) :
  QWidget(parent)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  int row = 0;

  layout->addWidget(new QLabel(tr("Target File Size (MB):")), row, 0);

  file_size_ = new FloatSlider();
  file_size_->SetMinimum(0);
  layout->addWidget(file_size_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Two-Pass")), row, 0);

  QCheckBox* two_pass_box = new QCheckBox();
  layout->addWidget(two_pass_box, row, 1);

  // File size defaults
  file_size_->SetValue(700.0);
}

int64_t H264FileSizeSection::GetFileSize() const
{
  // Convert megabytes to BITS
  return qRound64(file_size_->GetValue() * 1024.0 * 1024.0 * 8.0);
}

void H264FileSizeSection::SetFileSize(int64_t f)
{
  // Convert bits back to megabytes
  file_size_->SetValue(double(f) / 8.0 / 1024.0 / 1024.0);
}

H265Section::H265Section(QWidget *parent) :
  H264Section(H264CRFSection::kDefaultH265CRF, parent)
{
}

}
