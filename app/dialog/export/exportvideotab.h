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

#ifndef EXPORTVIDEOTAB_H
#define EXPORTVIDEOTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

#include "common/rational.h"
#include "dialog/export/codec/h264section.h"
#include "dialog/export/codec/imagesection.h"
#include "render/colormanager.h"
#include "widget/colorwheel/colorspacechooser.h"
#include "widget/slider/integerslider.h"

OLIVE_NAMESPACE_ENTER

class ExportVideoTab : public QWidget
{
  Q_OBJECT
public:
  ExportVideoTab(ColorManager* color_manager, QWidget* parent = nullptr);

  enum ScalingMethod {
    kFit,
    kStretch,
    kCrop
  };

  QComboBox* codec_combobox() const;

  IntegerSlider* width_slider() const;
  IntegerSlider* height_slider() const;
  QCheckBox* maintain_aspect_checkbox() const;
  QComboBox* scaling_method_combobox() const;

  const rational& frame_rate() const;
  void set_frame_rate(const rational& frame_rate);

  QString CurrentOCIOColorSpace();

  CodecSection* GetCodecSection() const;
  void SetCodecSection(CodecSection* section);
  ImageSection* image_section() const;
  H264Section* h264_section() const;

signals:
  void ColorSpaceChanged(const QString& colorspace);

private:
  QWidget* SetupResolutionSection();
  QWidget* SetupColorSection();
  QWidget* SetupCodecSection();

  QComboBox* codec_combobox_;
  QComboBox* frame_rate_combobox_;
  QCheckBox* maintain_aspect_checkbox_;
  QComboBox* scaling_method_combobox_;

  QStackedWidget* codec_stack_;
  ImageSection* image_section_;
  H264Section* h264_section_;

  ColorSpaceChooser* color_space_chooser_;

  IntegerSlider* width_slider_;
  IntegerSlider* height_slider_;

  QList<rational> frame_rates_;

  ColorManager* color_manager_;

private slots:
  void MaintainAspectRatioChanged(bool val);

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTVIDEOTAB_H
