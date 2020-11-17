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

#include "imagesection.h"

#include <QGridLayout>
#include <QLabel>

OLIVE_NAMESPACE_ENTER

ImageSection::ImageSection(QWidget* parent) :
  CodecSection(parent)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->setMargin(0);

  int row = 0;

  layout->addWidget(new QLabel(tr("Image Sequence:")), row, 0);

  image_sequence_checkbox_ = new QCheckBox();
  layout->addWidget(new QCheckBox(), row, 1);
}

QCheckBox *ImageSection::image_sequence_checkbox() const
{
  return image_sequence_checkbox_;
}

OLIVE_NAMESPACE_EXIT
