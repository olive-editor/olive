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

#ifndef AUDIO_H
#define AUDIO_H

#include <QVector>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QIODevice>
#include <QAudioOutput>
#include <QComboBox>

#include "timeline/sequence.h"

class AudioSenderThread : public QThread {
  Q_OBJECT
public:
  AudioSenderThread();
  void run();
  void stop();
  QWaitCondition cond;
  bool close;
  QMutex lock;
public slots:
  void notifyReceiver();
private:
  QVector<qint16> samples;
  int send_audio_to_output(qint64 offset, int max);
};

double log_volume(double linear);

extern QAudioOutput* audio_output;
extern QIODevice* audio_io_device;
extern AudioSenderThread* audio_thread;
extern QMutex audio_write_lock;

#define audio_ibuffer_size 192000
extern qint8 audio_ibuffer[audio_ibuffer_size];
extern qint64 audio_ibuffer_read;
extern long audio_ibuffer_frame;
extern double audio_ibuffer_timecode;
extern bool audio_scrub;
extern bool recording;
extern bool audio_rendering;
extern int audio_rendering_rate;
void clear_audio_ibuffer();

QObject *GetAudioWakeObject();
void SetAudioWakeObject(QObject* o);

int current_audio_freq();

bool is_audio_device_set();

void init_audio();
void stop_audio();
qint64 get_buffer_offset_from_frame(double framerate, long frame);

bool start_recording();
void stop_recording();
QString get_recorded_audio_filename();

void combobox_audio_sample_rates(QComboBox* combobox);

#endif // AUDIO_H
