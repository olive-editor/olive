#include "audiomonitor.h"

#include <QAudio>
#include <QDebug>
#include <QPainter>

#include "audio/audiomanager.h"
#include "common/qtversionabstraction.h"

const int kDecibelStep = 6;
const int kDecibelMinimum = -200;
const int kClearTimerInterval = 500;

AudioMonitor::AudioMonitor(QWidget *parent) :
  QWidget(parent)
{
  clear_timer_.setInterval(kClearTimerInterval);

  connect(AudioManager::instance(), SIGNAL(SentSamples(QVector<double>)), this, SLOT(SetValues(QVector<double>)));
  connect(&clear_timer_, SIGNAL(timeout()), this, SLOT(Clear()));
  connect(&clear_timer_, SIGNAL(timeout()), &clear_timer_, SLOT(stop()));
}

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

void AudioMonitor::Clear()
{
  values_.fill(0);
  update();
}

void AudioMonitor::paintEvent(QPaintEvent *)
{
  int channels = values_.size();

  if (channels == 0) {
    return;
  }

  QPainter p(this);
  QFontMetrics fm = p.fontMetrics();

  int peaks_y = 0;
  int peaks_height = fm.height();

  QRect db_labels_rect = rect();
  db_labels_rect.setWidth(QFontMetricsWidth(p.fontMetrics(), "-00"));
  db_labels_rect.adjust(0, peaks_height, 0, 0);

  QRect full_meter_rect = rect();
  full_meter_rect.adjust(db_labels_rect.width(), peaks_height, 0, 0);

  // Draw decibel markings
  QRect last_db_marking_rect;

  for (int i=0;i>=kDecibelMinimum;i-=kDecibelStep) {
    QString db_label;

    if (i <= kDecibelMinimum) {
      db_label = "-∞";
    } else {
      db_label = QString("%1").arg(i);
    }

    qreal log_val = QAudio::convertVolume(i, QAudio::DecibelVolumeScale, QAudio::LogarithmicVolumeScale);

    QRect db_marking_rect = db_labels_rect;
    db_marking_rect.adjust(0, db_labels_rect.height() - qRound(log_val * db_labels_rect.height()), 0, 0);
    db_marking_rect.setHeight(fm.height());

    // Prevent any dB markings overlapping
    if (i == 0 || !db_marking_rect.intersects(last_db_marking_rect)) {
      p.drawText(db_marking_rect, Qt::AlignRight, db_label);
      p.drawLine(db_marking_rect.topLeft(), db_marking_rect.topRight());

      last_db_marking_rect = db_marking_rect;
    }
  }

  QLinearGradient g(full_meter_rect.topLeft(), full_meter_rect.bottomLeft());
  g.setStops({
               QGradientStop(0.0, Qt::red),
               QGradientStop(0.25, Qt::yellow),
               QGradientStop(1.0, Qt::green)
             });

  int channel_width = full_meter_rect.width() / channels;

  for (int i=0;i<channels;i++) {
    int channel_x = full_meter_rect.x() + channel_width * i;

    QRect peaks_rect(channel_x, peaks_y, channel_width, peaks_height);

    QRect meter_rect = full_meter_rect;
    meter_rect.setX(channel_x);
    meter_rect.setWidth(channel_width);

    p.setPen(Qt::black);

    // Draw peak rects
    p.setBrush(Qt::red);
    p.drawRect(peaks_rect);

    // Draw gradient meter
    p.setBrush(g);
    p.drawRect(meter_rect);

    // Draw inverted semi-transparent black overlay depending on information
    p.setPen(Qt::NoPen);

    // Validate value and whether it peaked
    double vol = values_.at(i);
    if (vol > 1.0) {
      peaked_[i] = true;
    }

    // Convert val to logarithmic scale
    vol = QAudio::convertVolume(vol, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale);

    p.setBrush(QColor(0, 0, 0, 128));

    meter_rect.adjust(0, 0, 0, -qRound(meter_rect.height() * vol));
    p.drawRect(meter_rect);

    if (!peaked_.at(i))
      p.drawRect(peaks_rect);
  }
}

void AudioMonitor::mousePressEvent(QMouseEvent *)
{
  peaked_.fill(false);
  update();
}
