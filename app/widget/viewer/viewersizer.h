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

#ifndef VIEWERSIZER_H
#define VIEWERSIZER_H

#include <QWidget>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A container widget that enforces the aspect ratio of a child widget
 *
 * Using a provided width and height, this widget calculates the aspect ratio and forces the child widget to stay
 * confined to that aspect ratio and centered within the widget.
 *
 * The aspect ratio is calculated width divided by height. If the aspect ratio is zero (either width or height == 0),
 * the widget is hidden until a valid size is provided.
 */
class ViewerSizer : public QWidget
{
  Q_OBJECT
public:
  ViewerSizer(QWidget* parent = nullptr);

  /**
   * @brief Set the widget to be adjusted by this widget
   *
   * ViewerSizer takes ownership of this widget. If a widget was previously set, it is destroyed.
   */
  void SetWidget(QWidget* widget);

  /**
   * @brief Set resolution to use
   *
   * This is not the actual resolution of the viewer, it's used to calculate the aspect ratio
   */
  void SetChildSize(int width, int height);

  /**
   * @brief Set the zoom value of the child widget
   *
   * The number is an integer percentage (100 = 100%). Set to 0 to auto-fit.
   */
  void SetZoom(int percent);

signals:
  void RequestMatrix(const QMatrix4x4& matrix);

  /**
   * @brief Tells the viewerdisplay widget if the image is enlarged to be bigger than the widget or not.
   */
  void IsZoomed(bool flag);

protected:
  /**
   * @brief Listen for resize events to ensure the child widget remains correctly sized
   */
  virtual void resizeEvent(QResizeEvent *event) override;

private:
  /**
   * @brief Main sizing function, resizes widget_ to fit aspect_ratio_ (or hides if aspect ratio is 0)
   */
  void UpdateSize();

  /**
   * @brief Reference to widget
   *
   * If this is nullptr, all sizing operations are no-ops
   */
  QWidget* widget_;

  /**
   * @brief Internal resolution values
   */
  int width_;
  int height_;

  /**
   * @brief Aspect ratio calculated from the size provided by SetChildSize()
   */
  double aspect_ratio_;

  /**
   * @brief Internal zoom value
   */
  int zoom_;

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERSIZER_H
