/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "exportvideotab.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include "core.h"
#include "exportadvancedvideodialog.h"
#include "node/color/colormanager/colormanager.h"
#include "task/export/exportparams.h"

namespace olive {

ExportVideoTab::ExportVideoTab(ColorManager* color_manager, QWidget *parent) :
  QWidget(parent),
  color_manager_(color_manager),
  threads_(0)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  outer_layout->addWidget(SetupResolutionSection());

  outer_layout->addWidget(SetupCodecSection());

  outer_layout->addWidget(SetupColorSection());

  outer_layout->addStretch();
}

int ExportVideoTab::SetFormat(ExportFormat::Format format)
{
  format_ = format;

  QList<ExportCodec::Codec> vcodecs = ExportFormat::GetVideoCodecs(format);
  setEnabled(!vcodecs.isEmpty());
  codec_combobox()->clear();
  foreach (ExportCodec::Codec vcodec, vcodecs) {
    codec_combobox()->addItem(ExportCodec::GetCodecName(vcodec), vcodec);
  }
  return vcodecs.size();
}

bool ExportVideoTab::IsImageSequenceSet() const
{
  ImageSection* img_section = dynamic_cast<ImageSection*>(codec_stack_->currentWidget());

  return (img_section && img_section->IsImageSequenceChecked());
}

QWidget* ExportVideoTab::SetupResolutionSection()
{
  int row = 0;

  QGroupBox* resolution_group = new QGroupBox();
  resolution_group->setTitle(tr("General"));

  QGridLayout* layout = new QGridLayout(resolution_group);

  layout->addWidget(new QLabel(tr("Width:")), row, 0);

  width_slider_ = new IntegerSlider();
  width_slider_->SetMinimum(1);
  layout->addWidget(width_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Height:")), row, 0);

  height_slider_ = new IntegerSlider();
  height_slider_->SetMinimum(1);
  layout->addWidget(height_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Maintain Aspect Ratio:")), row, 0);

  maintain_aspect_checkbox_ = new QCheckBox();
  maintain_aspect_checkbox_->setChecked(true);
  layout->addWidget(maintain_aspect_checkbox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Scaling Method:")), row, 0);

  scaling_method_combobox_ = new QComboBox();
  scaling_method_combobox_->setEnabled(false);
  scaling_method_combobox_->addItem(tr("Fit"), ExportParams::kFit);
  scaling_method_combobox_->addItem(tr("Stretch"), ExportParams::kStretch);
  scaling_method_combobox_->addItem(tr("Crop"), ExportParams::kCrop);
  layout->addWidget(scaling_method_combobox_, row, 1);

  // Automatically enable/disable the scaling method depending on maintain aspect ratio
  connect(maintain_aspect_checkbox_, &QCheckBox::toggled, this, &ExportVideoTab::MaintainAspectRatioChanged);

  row++;

  layout->addWidget(new QLabel(tr("Frame Rate:")), row, 0);

  frame_rate_combobox_ = new FrameRateComboBox();
  connect(frame_rate_combobox_, &FrameRateComboBox::FrameRateChanged, this, &ExportVideoTab::UpdateFrameRate);
  layout->addWidget(frame_rate_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Pixel Aspect Ratio:")), row, 0);

  pixel_aspect_combobox_ = new PixelAspectRatioComboBox();
  layout->addWidget(pixel_aspect_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Interlacing:")), row, 0);

  interlaced_combobox_ = new InterlacedComboBox();
  layout->addWidget(interlaced_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Quality:")), row, 0);

  pixel_format_field_ = new PixelFormatComboBox(false);
  layout->addWidget(pixel_format_field_, row, 1);

  return resolution_group;
}

QWidget* ExportVideoTab::SetupColorSection()
{
  color_space_chooser_ = new ColorSpaceChooser(color_manager_, true, false);
  connect(color_space_chooser_, &ColorSpaceChooser::InputColorSpaceChanged, this, &ExportVideoTab::ColorSpaceChanged);
  return color_space_chooser_;
}

QWidget *ExportVideoTab::SetupCodecSection()
{
  int row = 0;

  QGroupBox* codec_group = new QGroupBox();
  codec_group->setTitle(tr("Codec"));

  QGridLayout* codec_layout = new QGridLayout(codec_group);

  codec_layout->addWidget(new QLabel(tr("Codec:")), row, 0);

  codec_combobox_ = new QComboBox();
  codec_layout->addWidget(codec_combobox_, row, 1);
  connect(codec_combobox_,
          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &ExportVideoTab::VideoCodecChanged);

  row++;

  codec_stack_ = new QStackedWidget();
  codec_layout->addWidget(codec_stack_, row, 0, 1, 2);

  image_section_ = new ImageSection();
  codec_stack_->addWidget(image_section_);

  h264_section_ = new H264Section();
  codec_stack_->addWidget(h264_section_);

  h265_section_ = new H265Section();
  codec_stack_->addWidget(h265_section_);

  row++;

  QPushButton* advanced_btn = new QPushButton(tr("Advanced"));
  connect(advanced_btn, &QPushButton::clicked, this, &ExportVideoTab::OpenAdvancedDialog);
  codec_layout->addWidget(advanced_btn, row, 1);

  return codec_group;
}

void ExportVideoTab::MaintainAspectRatioChanged(bool val)
{
  scaling_method_combobox_->setEnabled(!val);
}

void ExportVideoTab::OpenAdvancedDialog()
{
  // Find export formats compatible with this encoder
  QStringList pixel_formats = ExportFormat::GetPixelFormatsForCodec(format_, GetSelectedCodec());

  ExportAdvancedVideoDialog d(pixel_formats, this);

  d.set_threads(threads_);
  d.set_pix_fmt(pix_fmt_);

  if (d.exec() == QDialog::Accepted) {
    threads_ = d.threads();
    pix_fmt_ = d.pix_fmt();
  }
}

void ExportVideoTab::UpdateFrameRate(rational r)
{
  // Convert frame rate to timebase
  r.flip();

  for (int i=0; i<codec_stack_->count(); i++) {
    ImageSection* img = dynamic_cast<ImageSection*>(codec_stack_->widget(i));
    if (img) {
      img->SetTimebase(r);
    }
  }
}

void ExportVideoTab::VideoCodecChanged()
{
  ExportCodec::Codec codec = GetSelectedCodec();

  if (codec == ExportCodec::kCodecH264) {
    SetCodecSection(h264_section_);
  } else if (codec == ExportCodec::kCodecH265) {
    SetCodecSection(h265_section_);
  } else if (ExportCodec::IsCodecAStillImage(codec)) {
    SetCodecSection(image_section_);
  } else {
    SetCodecSection(nullptr);
  }

  // Set default pixel format
  QStringList pix_fmts = ExportFormat::GetPixelFormatsForCodec(format_, codec);
  if (!pix_fmts.isEmpty()) {
    pix_fmt_ = pix_fmts.first();
  } else {
    pix_fmt_.clear();
  }
  qDebug() << "Set default pix fmt" << pix_fmt_;
}

void ExportVideoTab::SetTimestamp(int64_t timestamp)
{
  for (int i=0; i<codec_stack_->count(); i++) {
    ImageSection* img = dynamic_cast<ImageSection*>(codec_stack_->widget(i));
    if (img) {
      img->SetTimestamp(timestamp);
    }
  }
}

}
