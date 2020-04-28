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

#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QWidget>

#include "viewerdisplay.h"

OLIVE_NAMESPACE_ENTER

class ViewerWindow : public QWidget
{
public:
  ViewerWindow(QWidget* parent = nullptr);

  ViewerDisplayWidget* gl_widget() const;

  /**
   * @brief Used to adjust resulting picture to be the right aspect ratio
   */
  void SetResolution(int width, int height);

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;

  virtual void closeEvent(QCloseEvent* e) override;

private:
  ViewerDisplayWidget* gl_widget_;

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERWINDOW_H
