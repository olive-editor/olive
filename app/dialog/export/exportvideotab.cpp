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

#include "exportvideotab.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include "core.h"
#include "exportadvancedvideodialog.h"
#include "render/backend/exportparams.h"
#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

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

QComboBox *ExportVideoTab::codec_combobox() const
{
  return codec_combobox_;
}

IntegerSlider *ExportVideoTab::width_slider() const
{
  return width_slider_;
}

IntegerSlider *ExportVideoTab::height_slider() const
{
  return height_slider_;
}

QCheckBox *ExportVideoTab::maintain_aspect_checkbox() const
{
  return maintain_aspect_checkbox_;
}

QComboBox *ExportVideoTab::scaling_method_combobox() const
{
  return scaling_method_combobox_;
}

const rational &ExportVideoTab::frame_rate() const
{
  return frame_rates_.at(frame_rate_combobox_->currentIndex());
}

void ExportVideoTab::set_frame_rate(const rational &frame_rate)
{
  frame_rate_combobox_->setCurrentIndex(frame_rates_.indexOf(frame_rate));
}

QString ExportVideoTab::CurrentOCIOColorSpace()
{
  return color_space_chooser_->input();
}

CodecSection *ExportVideoTab::GetCodecSection() const
{
  return static_cast<CodecSection*>(codec_stack_->currentWidget());
}

void ExportVideoTab::SetCodecSection(CodecSection *section)
{
  codec_stack_->setCurrentWidget(section);
}

ImageSection *ExportVideoTab::image_section() const
{
  return image_section_;
}

H264Section *ExportVideoTab::h264_section() const
{
  return h264_section_;
}

const int &ExportVideoTab::threads() const
{
  return threads_;
}

QWidget* ExportVideoTab::SetupResolutionSection()
{
  int row = 0;

  QGroupBox* resolution_group = new QGroupBox();
  resolution_group->setTitle(tr("Basic"));

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

  frame_rate_combobox_ = new QComboBox();
  frame_rates_ = Core::SupportedFrameRates();
  foreach (const rational& fr, frame_rates_) {
    frame_rate_combobox_->addItem(Core::FrameRateToString(fr));
  }

  layout->addWidget(frame_rate_combobox_, row, 1);

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

  row++;

  codec_stack_ = new QStackedWidget();
  codec_layout->addWidget(codec_stack_, row, 0, 1, 2);

  image_section_ = new ImageSection();
  codec_stack_->addWidget(image_section_);

  h264_section_ = new H264Section();
  codec_stack_->addWidget(h264_section_);

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
  ExportAdvancedVideoDialog d(this);

  d.set_threads(threads_);

  if (d.exec() == QDialog::Accepted) {
    threads_ = d.threads();
  }
}

OLIVE_NAMESPACE_EXIT
