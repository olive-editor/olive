#include "audiomonitor.h"

#include "project/sequence.h"
#include "playback/audio.h"
#include "panels/panels.h"
#include "panels/timeline.h"

#include <QPainter>
#include <QLinearGradient>
#include <QtMath>
#include <QDebug>

#define AUDIO_MONITOR_PEAK_HEIGHT 15
#define AUDIO_MONITOR_GAP 3

extern "C" {
#include "libavformat/avformat.h"
}

AudioMonitor::AudioMonitor(QWidget *parent) : QWidget(parent) {
    reset();
}

void AudioMonitor::reset() {
    sample_cache_offset = -1;
    sample_cache.clear();
    update();
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
		/*if (peaks.size() != channel_count) {
            peaks.resize(channel_count);
            peaks.fill(false);
		}*/
        int channel_width = (width()/channel_count) - AUDIO_MONITOR_GAP;
        long playhead_offset = -1;
        int i;
        for (i=0;i<channel_count;i++) {
            QRect r(channel_x, AUDIO_MONITOR_PEAK_HEIGHT + AUDIO_MONITOR_GAP, channel_width, height());
            p.fillRect(r, gradient);

			if (sample_cache_offset != -1 && sample_cache.size() > 0) {
                playhead_offset = (panel_timeline->playhead - sample_cache_offset) * channel_count;
				if (playhead_offset >= 0 && playhead_offset < sample_cache.size()) {
					double multiplier = 1 - qAbs((double) sample_cache.at(playhead_offset+i) / 32768.0); // 16-bit int divided to float
					/*if (multiplier == (double) 0) {
                        peaks[i] = true;
					}*/
                    r.setHeight(r.height()*multiplier);
                } else {
                    reset();
                }
            }

			/*QRect peak_rect(channel_x, 0, channel_width, AUDIO_MONITOR_PEAK_HEIGHT);
			if (peaks.at(i)) {
                p.fillRect(peak_rect, QColor(255, 0, 0));
            } else {
                p.fillRect(peak_rect, QColor(64, 0, 0));
			}*/

            p.fillRect(r, QColor(0, 0, 0, 160));

            channel_x += channel_width + AUDIO_MONITOR_GAP;
        }
        if (playhead_offset > -1) {
            // clean up used samples
            bool error = false;
            while (sample_cache_offset < panel_timeline->playhead && !error) {
                sample_cache_offset++;
                for (i=0;i<channel_count;i++) {
                    if (sample_cache.isEmpty()) {
                        reset();
                        error = true;
                        break;
                    } else {
                        sample_cache.removeFirst();
                    }
                }
            }
        }
    }
}
