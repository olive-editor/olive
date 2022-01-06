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

#ifndef SCROLLINGLABEL_H
#define SCROLLINGLABEL_H

#include <QTimer>
#include <QWidget>

namespace olive {

class ScrollingLabel : public QWidget
{
  Q_OBJECT
public:
  ScrollingLabel(QWidget* parent = nullptr);
  ScrollingLabel(const QStringList& text, QWidget* parent = nullptr);

  void SetText(const QStringList& text);

  void StartAnimating()
  {
    timer_.start();
  }

  void StopAnimating()
  {
    timer_.stop();
  }

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private:
  static void SetOpacityOfScanLine(uchar* scan_line, int width, int channels, double mul);

  static const int kMinLineHeight;

  QStringList text_;

  int text_height_;

  QTimer timer_;

  int animate_;

private slots:
  void AnimationUpdate();

};

}

#endif // SCROLLINGLABEL_H
