/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef SLIDERLABEL_H
#define SLIDERLABEL_H

#include <QLabel>

#include "common/define.h"

namespace olive {

class SliderLabel : public QLabel
{
  Q_OBJECT
public:
  SliderLabel(QWidget* parent);

  void SetColor(const QColor &c);

protected:
  virtual void mousePressEvent(QMouseEvent *e) override;

  virtual void mouseReleaseEvent(QMouseEvent *e) override;

  virtual void focusInEvent(QFocusEvent *event) override;

  virtual void changeEvent(QEvent *event) override;

signals:
  void LabelPressed();

  void LabelReleased();

  void focused();

  void RequestReset();

  void ChangeSliderType();

private:
  bool override_color_enabled_;
  QColor override_color_;

};

}

#endif // SLIDERLABEL_H
