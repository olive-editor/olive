/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include <QApplication>
#include <QDebug>
#include <QPainter>

#include "audio/audiomanager.h"
#include "common/decibel.h"
#include "common/qtutils.h"

namespace olive {

const int kDecibelStep = 6;
const int kDecibelMinimum = -198; // Must be divisible by kDecibelStep for infinity to appear
const int kMaximumSmoothness = 8;

QVector<AudioMonitor*> AudioMonitor::instances_;

AudioMonitor::AudioMonitor(QWidget *parent) :
  QOpenGLWidget(parent),
  waveform_(nullptr),
  cached_channels_(0)
{
  instances_.append(this);

  values_.resize(kMaximumSmoothness);

  this->setMinimumWidth(this->fontMetrics().height());
}

AudioMonitor::~AudioMonitor()
{
  instances_.removeOne(this);
}

void AudioMonitor::SetParams(const AudioParams &params)
{
  if (params_ != params) {
    params_ = params;

    for (int i=0;i<values_.size();i++) {
      values_[i].resize(params_.channel_count());
      values_[i].fill(0);
    }

    peaked_.resize(params_.channel_count());
    peaked_.fill(false);

    update();
  }
}

void AudioMonitor::Stop()
{
  waveform_ = nullptr;

  // We don't stop the update loop here so that the monitor can show a smooth fade out. The update
  // loop will stop itself since file_ and waveform_ are null.
}

void AudioMonitor::PushSampleBuffer(const SampleBuffer &d)
{
  if (!params_.channel_count()) {
    return;
  }

  QVector<double> v(params_.channel_count(), 0);

  AudioVisualWaveform::Sample summed = AudioVisualWaveform::SumSamples(d, 0, d.sample_count());

  AudioVisualWaveformSampleToInternalValues(summed, v);

  // Fill values because they get averaged out for smoothing
  values_.fill(v);

  SetUpdateLoop(true);
}

void AudioMonitor::StartWaveform(const AudioWaveformCache *waveform, const rational &start, int playback_speed)
{
  Stop();

  waveform_length_ = waveform->length();
  if (start >= waveform_length_) {
    return;
  }

  waveform_ = waveform;
  waveform_time_ = start;

  playback_speed_ = playback_speed;

  last_time_ = QDateTime::currentMSecsSinceEpoch();

  SetUpdateLoop(true);
}

void AudioMonitor::SetUpdateLoop(bool e)
{
  if (e) {
    connect(this, &AudioMonitor::frameSwapped, this, static_cast<void(AudioMonitor::*)()>(&AudioMonitor::update));

    update();
  } else {
    disconnect(this, &AudioMonitor::frameSwapped, this, static_cast<void(AudioMonitor::*)()>(&AudioMonitor::update));
  }
}

void AudioMonitor::paintGL()
{
  QPainter p(this);
  QPalette palette = qApp->palette();
  QRect geometry(0, 0, width(), height());

  p.fillRect(geometry, palette.window().color());

  if (!params_.channel_count()) {
    return;
  }

  QFont f = p.font();
  f.setPointSizeF(f.pointSizeF() * 0.75);
  p.setFont(f);

  QFontMetrics fm = p.fontMetrics();

  int font_height = fm.height();

  bool horizontal = this->width() > this->height();

  int minimum_width = this->fontMetrics().height() * 3;

  // Determine rect where the main meter will go
  QRect full_meter_rect = geometry;

  // Create rect where decibel markings will go on the side
  QRect db_labels_rect;
  bool draw_text;

  // Width of each channel in the meter
  int peaks_pos;
  int channel_size;
  int db_line_length = fm.horizontalAdvance(QStringLiteral("-"));
  int db_width = QtUtils::QFontMetricsWidth(p.fontMetrics(), "-00 ");
  if (horizontal) {
    // Insert peaks area
    full_meter_rect.adjust(0, 0, -font_height, 0);

    draw_text = (height() >= minimum_width);

    // Derive dB label rect
    if (draw_text) {
      // Insert meter rect
      full_meter_rect.adjust(db_width/2, 0, 0, -font_height);

      db_labels_rect = full_meter_rect;
      db_labels_rect.setTop(db_labels_rect.bottom());
      db_labels_rect.setHeight(font_height + db_line_length);
    }

    // Divide height by channel count
    channel_size = full_meter_rect.height() / params_.channel_count();

    // Set peaks Y to right-most
    peaks_pos = full_meter_rect.right();
  } else {
    // Insert peaks rect
    full_meter_rect.adjust(0, font_height, 0, 0);

    draw_text = (width() >= minimum_width);

    // Derive dB label rect
    if (draw_text) {
      int db_width_and_line = db_width + db_line_length;

      // Insert meter rect
      full_meter_rect.adjust(db_width_and_line, 0, 0, -font_height/2);

      db_labels_rect = full_meter_rect;
      db_labels_rect.setX(0);
      db_labels_rect.setWidth(db_width_and_line);
    }

    // Divide width by channel count
    channel_size = full_meter_rect.width() / params_.channel_count();

    // Set peaks Y to very top
    peaks_pos = 0;
  }

  if (cached_background_.size() != size()
      || cached_channels_ != params_.channel_count()) {

    cached_channels_ = params_.channel_count();

    // Generate new background
    cached_background_ = QPixmap(size());
    cached_background_.fill(Qt::transparent);

    QPainter cached_painter(&cached_background_);

    if (draw_text) {
      cached_painter.setFont(f);

      // Draw decibel markings
      QRect last_db_marking_rect;

      cached_painter.setPen(palette.text().color());

      for (int i=0;i>=kDecibelMinimum;i-=kDecibelStep) {
        QString db_label;
        qreal log_val;

        if (i == kDecibelMinimum) {
          db_label = QStringLiteral("-∞ ");
          log_val = 0;
        } else {
          db_label = QStringLiteral("%1 ").arg(i);
          log_val = Decibel::toLogarithmic(i);
        }

        QLine db_line;
        QRect db_marking_rect;
        bool overlaps_infinity = false;

        if (horizontal) {
          int x = db_labels_rect.x() + qRound(log_val * db_labels_rect.width());

          db_marking_rect = QRect(x - db_width/2, db_labels_rect.top() + db_line_length, db_width, db_labels_rect.height() - db_line_length);
          db_line = QLine(x, db_labels_rect.top(), x, db_labels_rect.top() + db_line_length);

          overlaps_infinity = db_marking_rect.left() < db_labels_rect.left() + db_width/2;
        } else {
          int y = db_labels_rect.y() + db_labels_rect.height() - qRound(log_val * db_labels_rect.height());

          db_marking_rect = QRect(db_labels_rect.x(), y - font_height/2, db_labels_rect.width() - db_line_length, font_height);
          db_line = QLine(db_marking_rect.right()+1, y, db_labels_rect.width(), y);

          overlaps_infinity = db_marking_rect.bottom() >= db_labels_rect.bottom()-font_height;
        }

        if (overlaps_infinity && i == kDecibelMinimum) {
          overlaps_infinity = false;
        }

        // Prevent any dB markings overlapping
        if (i == 0 || (!db_marking_rect.intersects(last_db_marking_rect) && !overlaps_infinity)) {
          cached_painter.drawText(db_marking_rect, Qt::AlignCenter, db_label);
          cached_painter.drawLine(db_line);

          last_db_marking_rect = db_marking_rect;
        }
      }
    }

    {
      // Draw bars
      QLinearGradient g;

      if (horizontal) {
        g.setStart(full_meter_rect.topRight());
        g.setFinalStop(full_meter_rect.topLeft());
      } else {
        g.setStart(full_meter_rect.topLeft());
        g.setFinalStop(full_meter_rect.bottomLeft());
      }

      g.setStops({
                   QGradientStop(0.0, Qt::red),
                   QGradientStop(0.25, Qt::yellow),
                   QGradientStop(1.0, Qt::green)
                 });

      cached_painter.setPen(Qt::black);

      for (int i=0;i<params_.channel_count();i++) {
        QRect peaks_rect, meter_rect;

        if (horizontal) {
          int channel_y = full_meter_rect.y() + channel_size * i;

          peaks_rect = QRect(peaks_pos, channel_y, font_height, channel_size);

          meter_rect = full_meter_rect;
          meter_rect.setY(channel_y);
          meter_rect.setHeight(channel_size);
        } else {
          int channel_x = full_meter_rect.x() + channel_size * i;

          peaks_rect = QRect(channel_x, peaks_pos, channel_size, font_height);

          meter_rect = full_meter_rect;
          meter_rect.setX(channel_x);
          meter_rect.setWidth(channel_size);
        }

        // Draw peak rects
        cached_painter.setBrush(Qt::red);
        cached_painter.drawRect(peaks_rect);

        // Draw gradient meter
        cached_painter.setBrush(g);
        cached_painter.drawRect(meter_rect);
      }
    }
  }

  p.drawPixmap(0, 0, cached_background_);

  QVector<double> v(params_.channel_count(), 0);

  if (IsPlaying()) {
    // Determines how many milliseconds have passed since last update
    qint64 current_time = QDateTime::currentMSecsSinceEpoch();
    qint64 delta_time = current_time - last_time_;
    int abs_speed = qAbs(playback_speed_);

    // Multiply by speed if the speed is not 1
    if (abs_speed != 1) {
      delta_time *= abs_speed;
    }

    if (waveform_) {
      UpdateValuesFromWaveform(v, delta_time);

      if (waveform_time_ >= waveform_length_) {
        Stop();
      }
    }

    last_time_ = current_time;
  }

  PushValue(v);

  QVector<double> vals = GetAverages();

  p.setBrush(QColor(0, 0, 0, 128));
  p.setPen(Qt::NoPen);

  bool all_zeroes = true;

  for (int i=0;i<params_.channel_count();i++) {
    // Validate value and whether it peaked
    double vol = vals.at(i);
    if (vol > 1.0) {
      peaked_[i] = true;
    }

    if (all_zeroes && !qIsNull(vol)) {
      all_zeroes = false;
    }

    // Convert val to logarithmic scale
    vol = Decibel::LinearToLogarithmic(vol);

    QRect peaks_rect, meter_rect;

    if (horizontal) {
      int channel_y = full_meter_rect.y() + channel_size * i;

      peaks_rect = QRect(peaks_pos, channel_y, font_height, channel_size);

      meter_rect = full_meter_rect;
      meter_rect.setY(channel_y);
      meter_rect.setHeight(channel_size);
      meter_rect.adjust(qRound(meter_rect.width() * vol), 0, 0, 0);
    } else {
      int channel_x = full_meter_rect.x() + channel_size * i;

      peaks_rect = QRect(channel_x, peaks_pos, channel_size, font_height);

      meter_rect = full_meter_rect;
      meter_rect.setX(channel_x);
      meter_rect.setWidth(channel_size);
      meter_rect.adjust(0, 0, 0, -qRound(meter_rect.height() * vol));
    }
    p.drawRect(meter_rect);

    if (!peaked_.at(i)) {
      p.drawRect(peaks_rect);
    }
  }

  if (all_zeroes && !IsPlaying()) {
    // Optimize by disabling the update loop
    SetUpdateLoop(false);
  }
}

void AudioMonitor::mousePressEvent(QMouseEvent *)
{
  peaked_.fill(false);
  update();
}

void AudioMonitor::UpdateValuesFromWaveform(QVector<double> &v, qint64 delta_time)
{
  // Delta time is provided in milliseconds, so we convert to seconds in rational
  rational length(delta_time, 1000);

  AudioVisualWaveform::Sample sum = waveform_->GetSummaryFromTime(waveform_time_, length);

  AudioVisualWaveformSampleToInternalValues(sum, v);

  waveform_time_ += length;
}

void AudioMonitor::AudioVisualWaveformSampleToInternalValues(const AudioVisualWaveform::Sample &in, QVector<double> &out)
{
  for (size_t i=0; i<in.size(); i++) {
    float max = qMax(qAbs(in.at(i).min), qAbs(in.at(i).max));

    int output_index = i%out.size();
    if (max > out.at(output_index)) {
      out[output_index] = max;
    }
  }
}

void AudioMonitor::PushValue(const QVector<double> &v)
{
  int lim = values_.size()-1;
  for (int i=0; i<lim; i++) {
    values_[i] = values_[i+1];
  }
  values_[lim] = v;
}

void AudioMonitor::BytesToSampleSummary(const QByteArray &b, QVector<double> &v)
{
  const float* samples = reinterpret_cast<const float*>(b.constData());
  int nb_samples = b.size() / sizeof(float);

  for (int i=0;i<nb_samples;i++) {
    int channel = i % params_.channel_count();

    float abs_sample = samples[i];

    if (abs_sample > v.at(channel)) {
      v.replace(channel, abs_sample);
    }
  }
}

QVector<double> AudioMonitor::GetAverages() const
{
  QVector<double> v(params_.channel_count(), 0);

  for (int i=0;i<values_.size();i++) {
    for (int j=0;j<v.size();j++) {
      v[j] += values_.at(i).at(j);
    }
  }

  for (int i=0;i<v.size();i++) {
    v[i] /= static_cast<double>(values_.size());
  }

  return v;
}

}
