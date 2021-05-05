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

#include "imagesection.h"

#include <QGridLayout>
#include <QLabel>

namespace olive {

ImageSection::ImageSection(QWidget* parent) :
  CodecSection(parent)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->setMargin(0);

  int row = 0;

  layout->addWidget(new QLabel(tr("Image Sequence:")), row, 0);

  image_sequence_checkbox_ = new QCheckBox();
  connect(image_sequence_checkbox_, &QCheckBox::toggled, this, &ImageSection::ImageSequenceCheckBoxToggled);
  layout->addWidget(image_sequence_checkbox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Frame to Export:")), row, 0);

  frame_slider_ = new TimeSlider();
  frame_slider_->SetMinimum(0);
  frame_slider_->SetValue(0);
  layout->addWidget(frame_slider_, row, 1);
}

void ImageSection::ImageSequenceCheckBoxToggled(bool e)
{
  frame_slider_->setEnabled(!e);
}

}
