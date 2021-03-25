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

#include "colorlabelmenuitem.h"

#include <QHBoxLayout>

#include "ui/style/style.h"

namespace olive {

ColorLabelMenuItem::ColorLabelMenuItem(QWidget* parent) :
  QWidget(parent)
{
  int text_height = fontMetrics().height();
  int padding = text_height/4;

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(padding);
  layout->setSpacing(padding);

  box_ = new ColorPreviewBox();
  box_->setFixedSize(text_height, text_height);
  layout->addWidget(box_);

  label_ = new QLabel();
  StyleManager::UseOSNativeStyling(label_);
  layout->addWidget(label_);
}

void ColorLabelMenuItem::SetText(const QString &text)
{
  label_->setText(text);
}

void ColorLabelMenuItem::SetColor(const Color &color)
{
  box_->SetColor(color);
}

}
