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

#ifndef IMAGESECTION_H
#define IMAGESECTION_H

#include <QCheckBox>

#include "codecsection.h"
#include "widget/slider/timeslider.h"

namespace olive {

class ImageSection : public CodecSection
{
  Q_OBJECT
public:
  ImageSection(QWidget* parent = nullptr);

  bool IsImageSequenceChecked() const
  {
    return image_sequence_checkbox_->isChecked();
  }

  void SetTimebase(const rational& r)
  {
    frame_slider_->SetTimebase(r);
  }

  int64_t GetTimestamp() const
  {
    return frame_slider_->GetValue();
  }

  void SetTimestamp(int64_t t)
  {
    frame_slider_->SetValue(t);
  }

private:
  QCheckBox* image_sequence_checkbox_;

  TimeSlider* frame_slider_;

private slots:
  void ImageSequenceCheckBoxToggled(bool e);

};

}

#endif // IMAGESECTION_H
