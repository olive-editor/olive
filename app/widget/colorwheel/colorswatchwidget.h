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

#ifndef COLORSWATCHWIDGET_H
#define COLORSWATCHWIDGET_H

#include <QOpenGLWidget>

#include "render/color.h"
#include "render/colorprocessor.h"

OLIVE_NAMESPACE_ENTER

class ColorSwatchWidget : public QWidget
{
  Q_OBJECT
public:
  ColorSwatchWidget(QWidget* parent = nullptr);

  const Color& GetSelectedColor() const;

  void SetColorProcessor(ColorProcessorPtr to_linear, ColorProcessorPtr to_display);

public slots:
  void SetSelectedColor(const Color& c);

signals:
  void SelectedColorChanged(const Color& c);

protected:
  virtual void mousePressEvent(QMouseEvent* e) override;

  virtual void mouseMoveEvent(QMouseEvent* e) override;

  virtual Color GetColorFromScreenPos(const QPoint& p) const = 0;

  virtual void SelectedColorChangedEvent(const Color& c, bool external);

  Qt::GlobalColor GetUISelectorColor() const;

  Color GetManagedColor(const Color& input) const;

private:
  void SetSelectedColorInternal(const Color& c, bool external);

  Color selected_color_;

  ColorProcessorPtr to_linear_processor_;

  ColorProcessorPtr to_display_processor_;

};

OLIVE_NAMESPACE_EXIT

#endif // COLORSWATCHWIDGET_H
