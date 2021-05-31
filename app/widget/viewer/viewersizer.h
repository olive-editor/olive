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

#ifndef VIEWERSIZER_H
#define VIEWERSIZER_H

#include <QScrollBar>
#include <QWidget>

#include "common/define.h"
#include "common/rational.h"

namespace olive {

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
   * @brief Set pixel aspect ratio
   */
  void SetPixelAspectRatio(const rational& pixel_aspect);

  /**
   * @brief Set the zoom value of the child widget
   *
   * The number is an integer percentage (100 = 100%). Set to 0 to auto-fit.
   */
  void SetZoom(int percent);

public slots:
  void HandDragMove(int x, int y);

signals:
  void RequestScale(const QMatrix4x4& matrix);

  void RequestTranslate(const QMatrix4x4& matrix);

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

  int GetZoomedValue(int value);

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

  rational pixel_aspect_;

  /**
   * @brief Internal zoom value
   */
  int zoom_;

  QScrollBar* horiz_scrollbar_;
  QScrollBar* vert_scrollbar_;

private slots:
  void ScrollBarMoved();

};

}

#endif // VIEWERSIZER_H
