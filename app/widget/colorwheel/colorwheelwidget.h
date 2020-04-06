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

#ifndef COLORWHEELWIDGET_H
#define COLORWHEELWIDGET_H

#include <QOpenGLWidget>

#include "colorswatchwidget.h"
#include "render/backend/opengl/openglshader.h"
#include "render/color.h"

OLIVE_NAMESPACE_ENTER

class ColorWheelWidget : public ColorSwatchWidget
{
  Q_OBJECT
public:
  ColorWheelWidget(QWidget* parent = nullptr);

signals:
  void DiameterChanged(int radius);

protected:
  virtual Color GetColorFromScreenPos(const QPoint& p) const override;

  virtual void resizeEvent(QResizeEvent* e) override;

  virtual void paintEvent(QPaintEvent* e) override;

  virtual void SelectedColorChangedEvent(const Color& c, bool external) override;

private:
  int GetDiameter() const;

  qreal GetRadius() const;

  struct Triangle {
    qreal opposite;
    qreal adjacent;
    qreal hypotenuse;
  };

  Triangle GetTriangleFromCoords(const QPoint &center, const QPoint& p) const;
  Triangle GetTriangleFromCoords(const QPoint &center, qreal y, qreal x) const;

  Color GetColorFromTriangle(const Triangle& tri) const;
  QPoint GetCoordsFromColor(const Color& c) const;

  QPixmap cached_wheel_;

  float val_;

  bool force_redraw_;

};

OLIVE_NAMESPACE_EXIT

#endif // COLORWHEELWIDGET_H
