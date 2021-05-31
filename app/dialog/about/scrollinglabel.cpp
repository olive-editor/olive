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

#include "scrollinglabel.h"

#include <QPainter>

#include "common/qtutils.h"

namespace olive {

const int ScrollingLabel::kMinLineHeight = 5;

ScrollingLabel::ScrollingLabel(QWidget *parent) :
  QWidget(parent),
  animate_(0)
{
  timer_.setInterval(50);
  connect(&timer_, &QTimer::timeout, this, &ScrollingLabel::AnimationUpdate);
}

ScrollingLabel::ScrollingLabel(const QStringList &text, QWidget *parent) :
  ScrollingLabel(parent)
{
  SetText(text);
}

void ScrollingLabel::SetText(const QStringList &text)
{
  text_ = text;

  QFontMetrics fm = fontMetrics();
  text_height_ = fm.height();

  int width = 0;
  foreach (const QString& s, text_) {
    width = qMax(width, QtUtils::QFontMetricsWidth(fm, s));
  }

  setMinimumSize(width, text_height_ * kMinLineHeight);
}

void ScrollingLabel::paintEvent(QPaintEvent *e)
{
  QImage map(width(), height(), QImage::Format_RGBA8888_Premultiplied);
  map.fill(Qt::transparent);

  {
    QPainter p(&map);
    p.setPen(palette().text().color());

    QFontMetrics fm = p.fontMetrics();
    int half_width = width();

    for (int i=0; i<text_.size(); i++) {
      int text_y = fm.ascent() + height() - animate_ + (fm.height()*i);
      int text_bottom = text_y + fm.descent();
      int text_top = text_y - fm.ascent();

      if (text_bottom < 0 || text_top >= height()) {
        continue;
      }

      const QString& s = text_.at(i);

      int width = QtUtils::QFontMetricsWidth(fm, s);
      p.drawText(half_width/2 - width/2, text_y, s);
    }

    for (int y=0; y<text_height_; y++) {
      double mul = double(y)/double(text_height_);

      SetOpacityOfScanLine(map.scanLine(y), map.width(), 4, mul);
      SetOpacityOfScanLine(map.scanLine(map.height()-1-y), map.width(), 4, mul);
    }
  }

  QPainter wp(this);
  wp.drawImage(0, 0, map);
}

void ScrollingLabel::SetOpacityOfScanLine(uchar *scan_line, int width, int channels, double mul)
{
  for (int x=0; x<width; x++) {
    uchar* pixel = &scan_line[x*4];

    for (int c=0; c<channels; c++) {
      pixel[c] *= mul;
    }
  }
}

void ScrollingLabel::AnimationUpdate()
{
  animate_++;

  if (animate_ >= height() + text_.size() * text_height_) {
    animate_ = 0;
  }

  update();
}

}
