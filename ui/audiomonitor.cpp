#include "audiomonitor.h"

#include "project/sequence.h"
#include "playback/audio.h"
#include "panels/panels.h"
#include "panels/timeline.h"

#include <QPainter>
#include <QLinearGradient>
#include <QtMath>
#include <QDebug>

#define AUDIO_MONITOR_GAP 3

extern "C" {
#include "libavformat/avformat.h"
}

AudioMonitor::AudioMonitor(QWidget *parent) : QWidget(parent) {
}

void AudioMonitor::reset() {
    sample_cache_offset = -1;
    sample_cache.clear();
}

void AudioMonitor::resizeEvent(QResizeEvent *e) {
    gradient = QLinearGradient(QPoint(0, rect().top()), QPoint(0, rect().bottom()));
    gradient.setColorAt(0, Qt::red);
    gradient.setColorAt(0.25, Qt::yellow);
    gradient.setColorAt(1, Qt::green);
    QWidget::resizeEvent(e);
}

void AudioMonitor::paintEvent(QPaintEvent *) {
    if (sequence != NULL) {
        QPainter p(this);
        int channel_x = AUDIO_MONITOR_GAP;
        int channel_count = av_get_channel_layout_nb_channels(sequence->audio_layout);
        int channel_width = (width()/channel_count) - AUDIO_MONITOR_GAP;
//        for (int i=0;i<channel_count;i++) {
        for (int i=0;i<1;i++) {
            QRect r(channel_x, 0, channel_width, height());
            p.fillRect(r, gradient);

            if (sample_cache_offset != -1) {
                long playhead_offset = panel_timeline->playhead - sample_cache_offset;
                if (playhead_offset >= 0 && playhead_offset < sample_cache.size()) {
                    //int channel_offset = i*2;
                    double multiplier = 1 - qAbs((double) sample_cache.at(playhead_offset) / 32768.0); // 16-bit int divided to float
                    r.setHeight(r.height()*multiplier);
                    sample_cache_offset++;
                    sample_cache.removeFirst();
                } else {
                    reset();
                }
            }

            p.fillRect(r, QColor(0, 0, 0, 160));
            channel_x += channel_width + AUDIO_MONITOR_GAP;
        }
    }
}
