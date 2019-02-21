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

AudioMonitor::AudioMonitor(QWidget *parent) :
	QWidget(parent)
{
	clear_timer.setInterval(500);
	connect(&clear_timer, SIGNAL(timeout()), this, SLOT(clear()));
}

void AudioMonitor::set_value(const QVector<double> &ivalues) {
	values = ivalues;
	update();

	QMetaObject::invokeMethod(&clear_timer, "start", Qt::QueuedConnection);
}

void AudioMonitor::clear() {
	clear_timer.stop();

	values.fill(1);
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
	if (olive::ActiveSequence != nullptr && values.size() > 0) {
		QPainter p(this);
		int channel_x = AUDIO_MONITOR_GAP;
		int channel_count = values.size();
		int channel_width = (width()/channel_count) - AUDIO_MONITOR_GAP;
		int i;
		for (i=0;i<channel_count;i++) {
			QRect r(channel_x, AUDIO_MONITOR_PEAK_HEIGHT + AUDIO_MONITOR_GAP, channel_width, height());
			p.fillRect(r, gradient);

			bool peak = false;

			r.setHeight(qRound(r.height()*(values.at(i))));
			peak = (r.height() == 0);

			QRect peak_rect(channel_x, 0, channel_width, AUDIO_MONITOR_PEAK_HEIGHT);
			if (peak) {
				p.fillRect(peak_rect, QColor(255, 0, 0));
			} else {
				p.fillRect(peak_rect, QColor(64, 0, 0));
			}

			p.fillRect(r, QColor(0, 0, 0, 160));

			channel_x += channel_width + AUDIO_MONITOR_GAP;
		}
	}
}
