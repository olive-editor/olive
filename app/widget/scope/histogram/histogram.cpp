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

#include "histogram.h"

#include <QPainter>
#include <QtMath>

#include "common/clamp.h"
#include "common/functiontimer.h"

OLIVE_NAMESPACE_ENTER

HistogramScope::HistogramScope(QWidget* parent) :
  ManagedDisplayWidget(parent),
  buffer_(nullptr)
{
  EnableDefaultContextMenu();

  connect(&worker_, &HistogramScopeWorker::Finished, this, &HistogramScope::FinishedProcessing, Qt::QueuedConnection);
  worker_.start(QThread::IdlePriority);
}

HistogramScope::~HistogramScope()
{
  worker_.Cancel();
  worker_.quit();
  worker_.wait();
}

void HistogramScope::SetBuffer(Frame* frame)
{
  buffer_ = frame;

  StartUpdate();
}

void HistogramScope::FinishedProcessing(QVector<double> red, QVector<double> green, QVector<double> blue)
{
  red_val_ = red;
  green_val_ = green;
  blue_val_ = blue;

  update();
}

void HistogramScope::paintGL()
{
  QVector<QLine> red_lines(red_val_.size());
  QVector<QLine> green_lines(green_val_.size());
  QVector<QLine> blue_lines(blue_val_.size());

  for (int i=0;i<red_lines.size();i++) {
    red_lines.replace(i, QLine(i, height() * (1.0 - red_val_.at(i)), i, height()));
    green_lines.replace(i, QLine(i, height() * (1.0 - green_val_.at(i)), i, height()));
    blue_lines.replace(i, QLine(i, height() * (1.0 - blue_val_.at(i)), i, height()));
  }

  QPainter p(this);
  p.setCompositionMode(QPainter::CompositionMode_Plus);

  p.setPen(Qt::red);
  p.drawLines(red_lines);

  p.setPen(Qt::green);
  p.drawLines(green_lines);

  p.setPen(Qt::blue);
  p.drawLines(blue_lines);
}

void HistogramScope::resizeEvent(QResizeEvent *e)
{
  QOpenGLWidget::resizeEvent(e);

  StartUpdate();
}

void HistogramScope::ColorProcessorChangedEvent()
{
  StartUpdate();
}

void HistogramScope::StartUpdate()
{
  if (buffer_) {
    worker_.QueueNext(*buffer_, color_service(), width());
  } else {
    // Update with nothing
    red_val_.clear();
    green_val_.clear();
    blue_val_.clear();
    update();
  }
}

HistogramScopeWorker::HistogramScopeWorker() :
  cancelled_(false)
{
}

void HistogramScopeWorker::run()
{
  while (!cancelled_) {
    next_lock_.lock();
    while (!next_.is_allocated()) {
      next_wait_.wait(&next_lock_);

      if (cancelled_){
        next_lock_.unlock();
        return;
      }
    }

    // Copy values
    Frame f = next_;
    int w = next_width_;
    ColorProcessorPtr processor = next_processor_;
    next_.destroy();

    next_lock_.unlock();

    // Color manage frame

    QVector<int> data(w * kRGBChannels, 0);

    int max_w = w-1;

    for (int x=0;x<f.width();x++) {
      for (int y=0;y<f.height();y++) {
        Color c = f.get_pixel(x, y);

        if (processor) {
          c = processor->ConvertColor(c);
        }

        data[qFloor(clamp(c.red(), 0.0f, 1.0f) * max_w)]++;
        data[qFloor(clamp(c.green(), 0.0f, 1.0f) * max_w) + w]++;
        data[qFloor(clamp(c.blue(), 0.0f, 1.0f) * max_w) + w * 2]++;
      }
    }

    int max_val = 0;

    foreach (const int& i, data) {
      if (i > max_val) {
        max_val = i;
      }
    }

    if (!max_val) {
      // Prevent divide by zero
      return;
    }

    QVector<double> red_lines(w);
    QVector<double> green_lines(w);
    QVector<double> blue_lines(w);

    for (int i=0;i<w;i++) {
      red_lines.replace(i, static_cast<double>(data.at(i)) / static_cast<double>(max_val));
      green_lines.replace(i, static_cast<double>(data.at(i + w)) / static_cast<double>(max_val));
      blue_lines.replace(i, static_cast<double>(data.at(i + w * 2)) / static_cast<double>(max_val));
    }

    emit Finished(red_lines, green_lines, blue_lines);
  }
}

void HistogramScopeWorker::QueueNext(const Frame &f, ColorProcessorPtr processor, int width)
{
  next_lock_.lock();

  next_ = f;
  next_width_ = width;
  next_processor_ = processor;

  next_wait_.wakeOne();

  next_lock_.unlock();
}

void HistogramScopeWorker::Cancel()
{
  cancelled_ = true;
  next_lock_.lock();
  next_wait_.wakeOne();
  next_lock_.unlock();
}

OLIVE_NAMESPACE_EXIT
