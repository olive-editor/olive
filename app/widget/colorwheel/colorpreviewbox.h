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

#ifndef COLORPREVIEWBOX_H
#define COLORPREVIEWBOX_H

#include <QWidget>

#include "render/color.h"
#include "render/colorprocessor.h"

namespace olive {

class ColorPreviewBox : public QWidget
{
  Q_OBJECT
public:
  ColorPreviewBox(QWidget* parent = nullptr);

  void SetColorProcessor(ColorProcessorPtr to_ref, ColorProcessorPtr to_display);

public slots:
  void SetColor(const Color& c);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private:
  Color color_;

  ColorProcessorPtr to_ref_processor_;

  ColorProcessorPtr to_display_processor_;

};

}

#endif // COLORPREVIEWBOX_H
