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

#include "audiomonitor.h"

#include <QAudio>
#include <QDebug>
#include <QPainter>

#include "audio/audiomanager.h"
#include "common/qtutils.h"

OLIVE_NAMESPACE_ENTER

const int kDecibelStep = 6;
const int kDecibelMinimum = -200;
const int kMaximumSmoothness = 8;

AudioMonitor::AudioMonitor(QWidget *parent) :
  QOpenGLWidget(parent),
  cached_channels_(0)
{
  values_.resize(kMaximumSmoothness);

  connect(AudioManager::instance(), &AudioManager::OutputDeviceStarted, this, &AudioMonitor::OutputDeviceSet);
  connect(AudioManager::instance(), &AudioManager::OutputPushed, this, &AudioMonitor::OutputPushed);
  connect(AudioManager::instance(), &AudioManager::AudioParamsChanged, this, &AudioMonitor::SetParams);
  connect(AudioManager::instance(), &AudioManager::Stopped, this, &AudioMonitor::Stop);
}

AudioMonitor::~AudioMonitor()
{
  Stop();
}

void AudioMonitor::SetParams(const AudioRenderingParams &params)
{
  params_ = params;

  for (int i=0;i<values_.size();i++) {
    values_[i].resize(params_.channel_count());
    values_[i].fill(0);
  }

  peaked_.resize(params_.channel_count());
  peaked_.fill(false);
}

void AudioMonitor::OutputDeviceSet(const QString &filename, qint64 offset, int playback_speed)
{
  Stop();

  file_.setFileName(filename);

  if (!file_.open(QFile::ReadOnly)) {
    qWarning() << "Failed to open" << filename;
    return;
  }

  file_.seek(offset);

  playback_speed_ = playback_speed;

  last_time_ = QDateTime::currentMSecsSinceEpoch();

  SetUpdateLoop(true);
}

void AudioMonitor::Stop()
{
  if (file_.isOpen()) {
    file_.close();
  }
}

void AudioMonitor::OutputPushed(const QByteArray &data)
{
  QVector<double> v(params_.channel_count(), 0);

  BytesToSampleSummary(data, v);

  PushValue(v);

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

/*
void AudioMonitor::SetValues(QVector<double> values)
{
  values_ = values;

  if (values_.size() != peaked_.size()) {
    peaked_.resize(values_.size());
    peaked_.fill(false);
  }

  clear_timer_.stop();
  clear_timer_.start();

  update();
}
*/

void AudioMonitor::paintGL()
{
  QPainter p(this);
  p.fillRect(rect(), palette().window().color());

  if (!params_.channel_count()) {
    return;
  }

  QFontMetrics fm = p.fontMetrics();

  int peaks_y = 0;
  int font_height = fm.height();

  // Create rect where decibel markings will go on the side
  QRect db_labels_rect = rect();
  db_labels_rect.setWidth(QFontMetricsWidth(p.fontMetrics(), "-00"));
  db_labels_rect.adjust(0, font_height, 0, 0);

  // Determine rect where the main meter will go
  QRect full_meter_rect = rect();
  full_meter_rect.adjust(db_labels_rect.width(), font_height, 0, 0);

  // Width of each channel in the meter
  int channel_width = full_meter_rect.width() / params_.channel_count();

  if (cached_background_.size() != size()
      || cached_channels_ != params_.channel_count()) {

    cached_channels_ = params_.channel_count();

    // Generate new background
    cached_background_ = QPixmap(size());
    cached_background_.fill(Qt::transparent);

    QPainter cached_painter(&cached_background_);

    {
      // Draw decibel markings
      QRect last_db_marking_rect;

      cached_painter.setPen(palette().text().color());

      for (int i=0;i>=kDecibelMinimum;i-=kDecibelStep) {
        QString db_label;

        if (i <= kDecibelMinimum) {
          db_label = "-∞";
        } else {
          db_label = QStringLiteral("%1").arg(i);
        }

        qreal log_val = QAudio::convertVolume(i, QAudio::DecibelVolumeScale, QAudio::LogarithmicVolumeScale);

        QRect db_marking_rect = db_labels_rect;
        db_marking_rect.adjust(0, db_labels_rect.height() - qRound(log_val * db_labels_rect.height()), 0, 0);
        db_marking_rect.setHeight(fm.height());

        // Prevent any dB markings overlapping
        if (i == 0 || !db_marking_rect.intersects(last_db_marking_rect)) {
          cached_painter.drawText(db_marking_rect, Qt::AlignRight, db_label);
          cached_painter.drawLine(db_marking_rect.topLeft(), db_marking_rect.topRight());

          last_db_marking_rect = db_marking_rect;
        }
      }
    }

    {
      // Draw bars
      QLinearGradient g(full_meter_rect.topLeft(), full_meter_rect.bottomLeft());
      g.setStops({
                   QGradientStop(0.0, Qt::red),
                   QGradientStop(0.25, Qt::yellow),
                   QGradientStop(1.0, Qt::green)
                 });

      cached_painter.setPen(Qt::black);

      for (int i=0;i<params_.channel_count();i++) {
        int channel_x = full_meter_rect.x() + channel_width * i;

        QRect peaks_rect(channel_x, peaks_y, channel_width, font_height);

        QRect meter_rect = full_meter_rect;
        meter_rect.setX(channel_x);
        meter_rect.setWidth(channel_width);

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

  if (file_.isOpen()) {
    UpdateValuesFromFile(v);
  }

  PushValue(v);

  QVector<double> vals = GetAverages();

  p.setBrush(QColor(0, 0, 0, 128));
  p.setPen(Qt::NoPen);

  bool all_zeroes = true;

  for (int i=0;i<params_.channel_count();i++) {
    int channel_x = full_meter_rect.x() + channel_width * i;

    QRect peaks_rect(channel_x, peaks_y, channel_width, font_height);

    QRect meter_rect = full_meter_rect;
    meter_rect.setX(channel_x);
    meter_rect.setWidth(channel_width);

    // Validate value and whether it peaked
    double vol = vals.at(i);
    if (vol > 1.0) {
      peaked_[i] = true;
    }

    if (all_zeroes && !qIsNull(vol)) {
      all_zeroes = false;
    }

    // Convert val to logarithmic scale
    vol = QAudio::convertVolume(vol, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale);

    meter_rect.adjust(0, 0, 0, -qRound(meter_rect.height() * vol));
    p.drawRect(meter_rect);

    if (!peaked_.at(i)) {
      p.drawRect(peaks_rect);
    }
  }

  if (all_zeroes && !file_.isOpen()) {
    // Optimize by disabling the update loop
    SetUpdateLoop(false);
  }
}

void AudioMonitor::mousePressEvent(QMouseEvent *)
{
  peaked_.fill(false);
  update();
}

void AudioMonitor::UpdateValuesFromFile(QVector<double>& v)
{
  // Determines how many milliseconds have passed since last update
  qint64 current_time = QDateTime::currentMSecsSinceEpoch();
  qint64 time_passed = current_time - last_time_;

  // Determine how many bytes this is
  int bytes_to_read = params_.time_to_bytes(static_cast<double>(time_passed) * 0.001);

  QByteArray b = file_.read(bytes_to_read);;

  BytesToSampleSummary(b, v);

  last_time_ = current_time;
}

void AudioMonitor::PushValue(const QVector<double> &v)
{
  values_.removeFirst();
  values_.append(v);
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

OLIVE_NAMESPACE_EXIT
