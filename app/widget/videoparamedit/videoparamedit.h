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

#ifndef VIDEOPARAMEDIT_H
#define VIDEOPARAMEDIT_H

#include <QCheckBox>
#include <QLabel>
#include <QWidget>

#include "render/colormanager.h"
#include "render/videoparams.h"
#include "widget/slider/integerslider.h"
#include "widget/standardcombos/frameratecombobox.h"
#include "widget/standardcombos/interlacedcombobox.h"
#include "widget/standardcombos/pixelaspectratiocombobox.h"
#include "widget/standardcombos/pixelformatcombobox.h"
#include "widget/standardcombos/videodividercombobox.h"

namespace olive {

class VideoParamEdit : public QWidget
{
  Q_OBJECT
public:
  VideoParamEdit(QWidget* parent = nullptr);

  enum ParamMask {
    kNone = 0x0,
    kEnabled = 0x1,
    kWidthHeight = 0x2,
    kDepth = 0x4,
    kFrameRate = 0x8,
    kFormat = 0x10,
    kChannelCount = 0x20,
    kPixelAspect = 0x40,
    kInterlacing = 0x80,
    kDivider = 0x100,
    kStreamIndex = 0x200,
    kIsImageSequence = 0x400,
    kStartTime = 0x800,
    kEndTime = 0x1000,
    kPremultipliedAlpha = 0x2000,
    kColorspace = 0x4000,
    kFrameRateIsNotTimebase = 0x8000,
  };

  void SetParameterMask(uint64_t mask);

  VideoParams GetVideoParams() const;
  void SetVideoParams(const VideoParams& p);

  /**
   * @brief Set pointer to ColorManager
   *
   * Call this before calling SetVideoParams because it'll populate the colorspace list so it
   * can correctly be chosen from in the UI.
   */
  void SetColorManager(ColorManager* cm);

  int GetWidth() const
  {
    return width_slider_->GetValue();
  }

  void SetWidth(int w)
  {
    width_slider_->SetValue(w);
  }

  int GetHeight() const
  {
    return height_slider_->GetValue();
  }

  void SetHeight(int h)
  {
    height_slider_->SetValue(h);
  }

  rational GetFrameRate() const
  {
    return frame_rate_combobox_->GetFrameRate();
  }

  void SetFrameRate(const rational& r)
  {
    frame_rate_combobox_->SetFrameRate(r);
  }

  rational GetPixelAspectRatio() const
  {
    return pixel_aspect_combobox_->GetPixelAspectRatio();
  }

  void SetPixelAspectRatio(const rational& r)
  {
    pixel_aspect_combobox_->SetPixelAspectRatio(r);
  }

  VideoParams::Interlacing GetInterlaceMode() const
  {
    return interlaced_combobox_->GetInterlaceMode();
  }

  void SetInterlaceMode(VideoParams::Interlacing i)
  {
    interlaced_combobox_->SetInterlaceMode(i);
  }

signals:
  void Changed();

private:
  void SetChannelCount(int count);

  void SetVideoTypeComboBox(VideoParams::Type type);

  QLabel* enabled_lbl_;
  QCheckBox* enabled_box_;
  QLabel* width_lbl_;
  IntegerSlider* width_slider_;
  QLabel* height_lbl_;
  IntegerSlider* height_slider_;
  QLabel* depth_lbl_;
  IntegerSlider* depth_slider_;
  QLabel* frame_rate_lbl_;
  FrameRateComboBox* frame_rate_combobox_;
  QLabel* pixel_aspect_lbl_;
  PixelAspectRatioComboBox* pixel_aspect_combobox_;
  QLabel* interlaced_lbl_;
  InterlacedComboBox* interlaced_combobox_;
  QLabel* format_lbl_;
  PixelFormatComboBox* format_combobox_;
  QLabel* channel_count_lbl_;
  QComboBox* channel_count_combobox_;
  QLabel* divider_lbl_;
  VideoDividerComboBox* divider_combobox_;
  QLabel* stream_index_lbl_;
  IntegerSlider* stream_index_slider_;
  QLabel* video_type_lbl_;
  QComboBox* video_type_combobox_;
  QLabel* start_time_lbl_;
  IntegerSlider* start_time_slider_;
  QLabel* end_time_lbl_;
  IntegerSlider* end_time_slider_;
  QLabel* premultiplied_alpha_lbl_;
  QCheckBox* premultiplied_alpha_box_;
  QLabel* colorspace_lbl_;
  QComboBox* colorspace_combobox_;

  ColorManager* color_manager_;

  rational timebase_temp_;

  uint64_t mask_;

};

}

#endif // VIDEOPARAMEDIT_H
