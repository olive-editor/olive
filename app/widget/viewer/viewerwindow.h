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

#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QWidget>

#include "viewerdisplay.h"

namespace olive {

class ViewerWindow : public QWidget
{
public:
  ViewerWindow(QWidget* parent = nullptr);

  ViewerDisplayWidget* display_widget() const;

  /**
   * @brief Used to adjust resulting picture to be the right aspect ratio
   *
   * Equivalent to calling SetResolution and SetPixelAspectRatio, just slightly faster since we
   * only calculate the matrix once rather than twice.
   */
  void SetVideoParams(const VideoParams &params);

  /**
   * @brief Used to adjust resulting picture to be the right aspect ratio
   */
  void SetResolution(int width, int height);

  /**
   * @brief Used to adjust resulting picture to be the right aspect ratio
   */
  void SetPixelAspectRatio(const rational& pixel_aspect);

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;

  virtual void closeEvent(QCloseEvent* e) override;

private:
  void UpdateMatrix();

  int width_;

  int height_;

  ViewerDisplayWidget* display_widget_;

  rational pixel_aspect_;

};

}

#endif // VIEWERWINDOW_H
