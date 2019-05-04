/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "audiomonitor.h"

#include "timeline/sequence.h"
#include "rendering/audio.h"
#include "panels/panels.h"
#include "panels/timeline.h"

#include <QPainter>
#include <QLinearGradient>
#include <QtMath>

#define AUDIO_MONITOR_PEAK_HEIGHT 15
#define AUDIO_MONITOR_GAP 3

extern "C" {
#include "libavformat/avformat.h"
}

AudioMonitor::AudioMonitor(QWidget *parent) :
  QWidget(parent),
  peaked_(false)
{
  clear_timer.setInterval(500);
  connect(&clear_timer, SIGNAL(timeout()), this, SLOT(clear()));
}

void AudioMonitor::set_value(const QVector<float> &ivalues) {
  if (value_lock.tryLock()) {
    values = ivalues;

    if (peaked_.size() != values.size()) {
      peaked_.resize(values.size());
      peaked_.fill(false);
    }

    value_lock.unlock();

    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    QMetaObject::invokeMethod(&clear_timer, "start", Qt::QueuedConnection);
  }
}

void AudioMonitor::clear() {
  clear_timer.stop();
  peaked_.fill(false);
  values.fill(0.0);
  update();
}

void AudioMonitor::resizeEvent(QResizeEvent *e) {
  gradient = QLinearGradient(QPoint(0, rect().top()), QPoint(0, rect().bottom()));
  gradient.setColorAt(0, Qt::red);
  gradient.setColorAt(0.25, Qt::yellow);
  gradient.setColorAt(1, Qt::green);
  QWidget::resizeEvent(e);
}

void AudioMonitor::mousePressEvent(QMouseEvent *)
{
  peaked_.fill(false);
  update();
}

void AudioMonitor::paintEvent(QPaintEvent *) {
  value_lock.lock();
  if (values.size() > 0) {
    QPainter p(this);
    int channel_x = AUDIO_MONITOR_GAP;
    int channel_count = values.size();
    int channel_width = (width()/channel_count) - AUDIO_MONITOR_GAP;

    for (int i=0;i<channel_count;i++) {

      // Fill audio monitor with red-yellow-green gradient
      QRect r(channel_x, AUDIO_MONITOR_PEAK_HEIGHT + AUDIO_MONITOR_GAP, channel_width, height());
      p.fillRect(r, gradient);

      // Check if we've peaked
      if (!peaked_[i] && values.at(i) > 1.0f) {
        peaked_[i] = true;
      }

      // We draw an inverted dark shadow over the gradient to represent to are not lit up
      // TODO change to decibel representation
      float val = 1.0f - qMin(1.0f, values.at(i));
      r.setHeight(qRound(r.height()*val));

      QRect peak_rect(channel_x, 0, channel_width, AUDIO_MONITOR_PEAK_HEIGHT);
      if (peaked_[i]) {
        // We're peaked on this channel, so we light up the peak light
        p.fillRect(peak_rect, QColor(255, 0, 0));
      } else {
        p.fillRect(peak_rect, QColor(64, 0, 0));
      }

      p.fillRect(r, QColor(0, 0, 0, 160));

      channel_x += channel_width + AUDIO_MONITOR_GAP;
    }
  }
  value_lock.unlock();
}
