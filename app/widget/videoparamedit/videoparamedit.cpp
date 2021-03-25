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

#include "videoparamedit.h"

#include <QGridLayout>

namespace olive {

VideoParamEdit::VideoParamEdit(QWidget* parent) :
  QWidget(parent),
  color_manager_(nullptr),
  mask_(0)
{
  QGridLayout* layout = new QGridLayout(this);

  layout->setMargin(0);

  int row = 0;

  // Enabled
  enabled_lbl_ = new QLabel(tr("Enabled:"));
  layout->addWidget(enabled_lbl_, row, 0);
  enabled_box_ = new QCheckBox();
  connect(enabled_box_, &QCheckBox::clicked, this, &VideoParamEdit::Changed);
  layout->addWidget(enabled_box_, row, 1);

  row++;

  // Width
  width_lbl_ = new QLabel(tr("Width:"));
  layout->addWidget(width_lbl_, row, 0);

  width_slider_ = new IntegerSlider();
  width_slider_->SetMinimum(1);
  width_slider_->SetMaximum(32768);
  connect(width_slider_, &IntegerSlider::ValueChanged, this, &VideoParamEdit::Changed);
  layout->addWidget(width_slider_, row, 1);

  row++;

  // Height
  height_lbl_ = new QLabel(tr("Height:"));
  layout->addWidget(height_lbl_, row, 0);

  height_slider_ = new IntegerSlider();
  height_slider_->SetMinimum(1);
  height_slider_->SetMaximum(32768);
  connect(height_slider_, &IntegerSlider::ValueChanged, this, &VideoParamEdit::Changed);
  layout->addWidget(height_slider_, row, 1);

  row++;

  // Depth
  depth_lbl_ = new QLabel(tr("Depth:"));
  layout->addWidget(depth_lbl_, row, 0);

  depth_slider_ = new IntegerSlider();
  depth_slider_->SetMinimum(1);
  depth_slider_->SetMaximum(32768);
  connect(depth_slider_, &IntegerSlider::ValueChanged, this, &VideoParamEdit::Changed);
  layout->addWidget(depth_slider_, row, 1);

  row++;

  // Pixel Format
  format_lbl_ = new QLabel(tr("Format:"));
  layout->addWidget(format_lbl_, row, 0);
  format_combobox_ = new PixelFormatComboBox(true);
  connect(format_combobox_, static_cast<void (PixelFormatComboBox::*)(int)>(&PixelFormatComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(format_combobox_, row, 1);

  row++;

  // Frame Rate
  frame_rate_lbl_ = new QLabel(tr("Frame Rate:"));
  layout->addWidget(frame_rate_lbl_, row, 0);

  frame_rate_combobox_ = new FrameRateComboBox();
  connect(frame_rate_combobox_, static_cast<void (FrameRateComboBox::*)(int)>(&FrameRateComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(frame_rate_combobox_, row, 1);

  row++;

  // Pixel Aspect Ratio
  pixel_aspect_lbl_ = new QLabel(tr("Pixel Aspect Ratio:"));
  layout->addWidget(pixel_aspect_lbl_, row, 0);

  pixel_aspect_combobox_ = new PixelAspectRatioComboBox();
  connect(pixel_aspect_combobox_, static_cast<void (PixelAspectRatioComboBox::*)(int)>(&PixelAspectRatioComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(pixel_aspect_combobox_, row, 1);

  row++;

  // Interlacing
  interlaced_lbl_ = new QLabel(tr("Interlacing:"));
  layout->addWidget(interlaced_lbl_, row, 0);

  interlaced_combobox_ = new InterlacedComboBox();
  connect(interlaced_combobox_, static_cast<void (InterlacedComboBox::*)(int)>(&InterlacedComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(interlaced_combobox_, row, 1);

  row++;

  // Channel Count
  channel_count_lbl_ = new QLabel(tr("Channel Count:"));
  layout->addWidget(channel_count_lbl_, row, 0);

  channel_count_combobox_ = new QComboBox();
  channel_count_combobox_->addItem(tr("RGB"), VideoParams::kRGBChannelCount);
  channel_count_combobox_->addItem(tr("RGBA"), VideoParams::kRGBAChannelCount);
  connect(channel_count_combobox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(channel_count_combobox_, row, 1);

  row++;

  // Divider
  divider_lbl_ = new QLabel(tr("Divider:"));
  layout->addWidget(divider_lbl_, row, 0);

  divider_combobox_ = new VideoDividerComboBox();
  connect(divider_combobox_, static_cast<void (VideoDividerComboBox::*)(int)>(&VideoDividerComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(divider_combobox_, row, 1);

  row++;

  // Stream Index
  stream_index_lbl_ = new QLabel(tr("Stream Index:"));
  layout->addWidget(stream_index_lbl_, row, 0);
  stream_index_slider_ = new IntegerSlider();
  stream_index_slider_->SetMinimum(0);
  connect(stream_index_slider_, &IntegerSlider::ValueChanged, this, &VideoParamEdit::Changed);
  layout->addWidget(stream_index_slider_, row, 1);

  row++;

  // Video type
  video_type_lbl_ = new QLabel(tr("Video Type:"));
  layout->addWidget(video_type_lbl_, row, 0);
  video_type_combobox_ = new QComboBox();
  video_type_combobox_->addItem(tr("Video"), VideoParams::kVideoTypeVideo);
  video_type_combobox_->addItem(tr("Still"), VideoParams::kVideoTypeStill);
  video_type_combobox_->addItem(tr("Image Sequence"), VideoParams::kVideoTypeImageSequence);
  connect(video_type_combobox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(video_type_combobox_, row, 1);

  row++;

  // Start time (for image sequences)
  start_time_lbl_ = new QLabel(tr("Start Time"));
  layout->addWidget(start_time_lbl_, row, 0);

  start_time_slider_ = new IntegerSlider();
  start_time_slider_->SetMinimum(0);
  connect(start_time_slider_, &IntegerSlider::ValueChanged, this, &VideoParamEdit::Changed);
  layout->addWidget(start_time_slider_, row, 1);

  row++;

  // End time (for image sequences)
  end_time_lbl_ = new QLabel(tr("End Time"));
  layout->addWidget(end_time_lbl_, row, 0);

  end_time_slider_ = new IntegerSlider();
  end_time_slider_->SetMinimum(0);
  connect(end_time_slider_, &IntegerSlider::ValueChanged, this, &VideoParamEdit::Changed);
  layout->addWidget(end_time_slider_, row, 1);

  row++;

  // Premultiplied alpha
  premultiplied_alpha_lbl_ = new QLabel(tr("Premultiplied Alpha"));
  layout->addWidget(premultiplied_alpha_lbl_, row, 0);

  premultiplied_alpha_box_ = new QCheckBox();
  connect(premultiplied_alpha_box_, &QCheckBox::clicked, this, &VideoParamEdit::Changed);
  layout->addWidget(premultiplied_alpha_box_, row, 1);

  row++;

  // Colorspace
  colorspace_lbl_ = new QLabel(tr("Colorspace"));
  layout->addWidget(colorspace_lbl_, row, 0);

  colorspace_combobox_ = new QComboBox();
  connect(colorspace_combobox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &VideoParamEdit::Changed);
  layout->addWidget(colorspace_combobox_, row, 1);
}

void VideoParamEdit::SetParameterMask(uint64_t mask)
{
  width_lbl_->setVisible(mask & kWidthHeight);
  width_slider_->setVisible(mask & kWidthHeight);
  height_lbl_->setVisible(mask & kWidthHeight);
  height_slider_->setVisible(mask & kWidthHeight);

  depth_lbl_->setVisible(mask & kDepth);
  depth_slider_->setVisible(mask & kDepth);

  frame_rate_lbl_->setVisible(mask & kFrameRate);
  frame_rate_combobox_->setVisible(mask & kFrameRate);

  pixel_aspect_lbl_->setVisible(mask & kPixelAspect);
  pixel_aspect_combobox_->setVisible(mask & kPixelAspect);

  interlaced_lbl_->setVisible(mask & kInterlacing);
  interlaced_combobox_->setVisible(mask & kInterlacing);

  enabled_lbl_->setVisible(mask & kEnabled);
  enabled_box_->setVisible(mask & kEnabled);

  format_lbl_->setVisible(mask & kFormat);
  format_combobox_->setVisible(mask & kFormat);

  channel_count_lbl_->setVisible(mask & kChannelCount);
  channel_count_combobox_->setVisible(mask & kChannelCount);

  divider_lbl_->setVisible(mask & kDivider);
  divider_combobox_->setVisible(mask & kDivider);

  stream_index_lbl_->setVisible(mask & kStreamIndex);
  stream_index_slider_->setVisible(mask & kStreamIndex);

  video_type_lbl_->setVisible(mask & kIsImageSequence);
  video_type_combobox_->setVisible(mask & kIsImageSequence);

  start_time_lbl_->setVisible(mask & kStartTime);
  start_time_slider_->setVisible(mask & kStartTime);

  end_time_lbl_->setVisible(mask & kEndTime);
  end_time_slider_->setVisible(mask & kEndTime);

  premultiplied_alpha_lbl_->setVisible(mask & kPremultipliedAlpha);
  premultiplied_alpha_box_->setVisible(mask & kPremultipliedAlpha);

  colorspace_lbl_->setVisible(mask & kColorspace);
  colorspace_combobox_->setVisible(mask & kColorspace);
}

VideoParams VideoParamEdit::GetVideoParams() const
{
  VideoParams p;

  p.set_enabled(enabled_box_->isChecked());
  p.set_width(width_slider_->GetValue());
  p.set_height(height_slider_->GetValue());
  p.set_depth(depth_slider_->GetValue());

  p.set_frame_rate(frame_rate_combobox_->GetFrameRate());
  if (mask_ & kFrameRateIsNotTimebase) {
    // Frame rate editor will only edit the frame rate
    p.set_time_base(timebase_temp_);
  } else {
    p.set_time_base(frame_rate_combobox_->GetFrameRate().flipped());
  }

  p.set_pixel_aspect_ratio(pixel_aspect_combobox_->GetPixelAspectRatio());
  p.set_interlacing(interlaced_combobox_->GetInterlaceMode());
  p.set_format(format_combobox_->GetPixelFormat());
  p.set_channel_count(channel_count_combobox_->currentData().toInt());
  p.set_divider(divider_combobox_->GetDivider());
  p.set_stream_index(stream_index_slider_->GetValue());
  p.set_video_type(static_cast<VideoParams::Type>(video_type_combobox_->currentData().toInt()));
  p.set_start_time(start_time_slider_->GetValue());
  p.set_duration(end_time_slider_->GetValue() - start_time_slider_->GetValue() + 1);
  p.set_premultiplied_alpha(premultiplied_alpha_box_->isChecked());
  p.set_colorspace(colorspace_combobox_->currentData().toString());

  return p;
}

void VideoParamEdit::SetVideoParams(const VideoParams &p)
{
  blockSignals(true);

  enabled_box_->setChecked(p.enabled());
  width_slider_->SetValue(p.width());
  height_slider_->SetValue(p.height());
  depth_slider_->SetValue(p.depth());

  if (mask_ & kFrameRateIsNotTimebase) {
    // Frame rate editor will only edit the frame rate
    frame_rate_combobox_->SetFrameRate(p.frame_rate());
    timebase_temp_ = p.time_base();
  } else {
    // Frame rate editor will edit both frame rate and time base
    frame_rate_combobox_->SetFrameRate(p.time_base().flipped());
  }

  pixel_aspect_combobox_->SetPixelAspectRatio(p.pixel_aspect_ratio());
  interlaced_combobox_->SetInterlaceMode(p.interlacing());
  format_combobox_->SetPixelFormat(p.format());
  SetChannelCount(p.channel_count());
  divider_combobox_->SetDivider(p.divider());
  stream_index_slider_->SetValue(p.stream_index());
  SetVideoTypeComboBox(p.video_type());
  start_time_slider_->SetValue(p.start_time());
  end_time_slider_->SetValue(p.start_time() + p.duration() - 1);
  premultiplied_alpha_box_->setChecked(p.premultiplied_alpha());

  if (color_manager_) {
    // Assume colorspace box has been populated correctly
    for (int i=0; i<colorspace_combobox_->count(); i++) {
      if (colorspace_combobox_->itemData(i).toString() == p.colorspace()) {
        colorspace_combobox_->setCurrentIndex(i);
        break;
      }
    }
  } else {
    // Box is empty, fill with single option so that it gets preserved in GetVideoParams()
    colorspace_combobox_->clear();
    colorspace_combobox_->addItem(p.colorspace(), p.colorspace());
  }

  blockSignals(false);
}

void VideoParamEdit::SetColorManager(ColorManager *cm)
{
  color_manager_ = cm;

  // Re-populate colorspace combobox
  colorspace_combobox_->clear();

  if (color_manager_) {
    // Add default colorspace
    colorspace_combobox_->addItem(tr("Default (%1)").arg(color_manager_->GetDefaultInputColorSpace()), QString());

    // Add remaining
    QStringList spaces = color_manager_->ListAvailableColorspaces();
    foreach (const QString& s, spaces) {
      colorspace_combobox_->addItem(s, s);
    }
  }
}

void VideoParamEdit::SetChannelCount(int count)
{
  for (int i=0; i<channel_count_combobox_->count(); i++) {
    if (channel_count_combobox_->itemData(i).toInt() == count) {
      channel_count_combobox_->setCurrentIndex(i);
      break;
    }
  }
}

void VideoParamEdit::SetVideoTypeComboBox(VideoParams::Type type)
{
  for (int i=0; i<video_type_combobox_->count(); i++) {
    if (video_type_combobox_->itemData(i).toInt() == type) {
      video_type_combobox_->setCurrentIndex(i);
      break;
    }
  }
}

}
