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

class ViewerSizer : public QWidget
{
  Q_OBJECT
public:
  ViewerSizer(QWidget* parent = nullptr);

  /**
   * @brief Set the widget to be adjusted by this widget
   *
   * ViewerSizer takes ownership of this widget
   */
  void SetWidget(QWidget* widget);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

private:
  void UpdateSize();

  QWidget* widget_;

  double aspect_ratio_;

private slots:
  void SizeChangedSlot(int width, int height);

};

#endif // VIEWERSIZER_H
