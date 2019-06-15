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

#include "audio.h"

#include "global/global.h"

#include "timeline/sequence.h"

#include "panels/panels.h"

#include "global/config.h"
#include "ui/audiomonitor.h"
#include "rendering/renderfunctions.h"
#include "global/debug.h"

#include <QApplication>
#include <QAudioOutput>
#include <QAudioInput>
#include <QtMath>
#include <QFile>
#include <QDir>
#include <QComboBox>

extern "C" {
#include <libavcodec/avcodec.h>
}

QAudioOutput* audio_output;
QIODevice* audio_io_device;
bool audio_device_set = false;
bool audio_scrub = false;
QMutex audio_write_lock;
QAudioInput* audio_input = nullptr;
QFile output_recording;
bool recording = false;

int audio_rendering_rate = 0;

float audio_ibuffer[audio_ibuffer_size];
qint64 audio_ibuffer_read = 0;
long audio_ibuffer_frame = 0;
double audio_ibuffer_timecode = 0;

AudioSenderThread* audio_thread = nullptr;

bool is_audio_device_set() {
  return audio_device_set;
}

QAudioDeviceInfo get_audio_device(QAudio::Mode mode) {
  QList<QAudioDeviceInfo> devs = QAudioDeviceInfo::availableDevices(mode);

  // try to retrieve preferred device from config
  QString preferred_device = (mode == QAudio::AudioOutput) ? olive::config.preferred_audio_output : olive::config.preferred_audio_input;
  if (!preferred_device.isEmpty()) {
    for (int i=0;i<devs.size();i++) {
      // try to match available devices with preferred device
      if (devs.at(i).deviceName() == preferred_device) {
        return devs.at(i);
      }
    }
  }

  // if no preferred output is set, try to get the default device
  QAudioDeviceInfo default_device = (mode == QAudio::AudioOutput) ? QAudioDeviceInfo::defaultOutputDevice() : QAudioDeviceInfo::defaultInputDevice();
  if (!default_device.isNull()) {
    return default_device;
  }

  // if no default output could be retrieved, just use the first in the list
  if (devs.size() > 0) {
    return devs.at(0);
  }

  // couldn't find any audio devices, return null device
  return QAudioDeviceInfo();
}

void init_audio() {
  stop_audio();

  QAudioFormat audio_format;
  audio_format.setSampleRate(olive::config.audio_rate);
  audio_format.setChannelCount(2);
  audio_format.setSampleSize(32);
  audio_format.setCodec("audio/pcm");
  audio_format.setByteOrder(QAudioFormat::LittleEndian);
  audio_format.setSampleType(QAudioFormat::Float);

  QAudioDeviceInfo info = get_audio_device(QAudio::AudioOutput);

  // see if desired format can be used by the device, use nearest if not
  if (!info.isFormatSupported(audio_format)) {
    qWarning() << "Audio format is not supported by backend, using nearest";
    audio_format = info.nearestFormat(audio_format);
  }

  audio_output = new QAudioOutput(info, audio_format);
  audio_output->moveToThread(QApplication::instance()->thread());
  audio_output->setNotifyInterval(5);

  // connect
  audio_io_device = audio_output->start();
  if (audio_io_device == nullptr) {
    qWarning() << "Received nullptr audio device. No compatible audio output was found.";
  } else {
    audio_device_set = true;

    // start sender thread
    audio_thread = new AudioSenderThread();
    QObject::connect(audio_output, SIGNAL(notify()), audio_thread, SLOT(notifyReceiver()));
    audio_thread->start(QThread::TimeCriticalPriority);

    clear_audio_ibuffer();
  }
}

void stop_audio() {
  if (audio_device_set) {
    audio_thread->stop();

    audio_output->stop();
    delete audio_output;
    audio_device_set = false;
  }
}

void clear_audio_ibuffer() {
  if (audio_thread != nullptr) audio_thread->lock.lock();
  audio_write_lock.lock();
  memset(audio_ibuffer, 0, audio_ibuffer_size * sizeof(float));
  audio_ibuffer_read = 0;
  audio_write_lock.unlock();
  if (audio_thread != nullptr) audio_thread->lock.unlock();
}

int current_audio_freq() {
  return olive::Global->is_exporting()
      ? audio_rendering_rate : audio_output->format().sampleRate();
}

qint64 get_buffer_offset_from_frame(double framerate, long frame) {
  if (frame >= audio_ibuffer_frame) {
    int multiplier = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    return qFloor((double(frame - audio_ibuffer_frame)/framerate)*current_audio_freq())*multiplier;
  } else {
    qWarning() << "Invalid values passed to get_buffer_offset_from_frame" << frame << "<" << audio_ibuffer_frame;
    return 0;
  }
}

AudioSenderThread::AudioSenderThread() : close(false) {
  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void AudioSenderThread::stop() {
  close = true;
  cond.wakeAll();
  wait();
}

void AudioSenderThread::notifyReceiver() {
  cond.wakeAll();
}

void AudioSenderThread::run() {
  // start data loop
  send_audio_to_output(0, audio_ibuffer_size);

  lock.lock();
  while (true) {
    cond.wait(&lock);
    if (close) {
      break;
    } else if (panel_sequence_viewer->playing || panel_footage_viewer->playing || audio_scrub) {

      int adjusted_read_index = (audio_ibuffer_read%audio_ibuffer_size);
      int max_write = (audio_ibuffer_size - adjusted_read_index) * sizeof(float);
      int actual_write = send_audio_to_output(adjusted_read_index, max_write);
      if (actual_write == max_write) {
        // got all the bytes, write again
        send_audio_to_output(0, audio_ibuffer_size);
      }

      audio_scrub = false;
    }
  }
  lock.unlock();
}

int AudioSenderThread::send_audio_to_output(qint64 offset, int max) {
  // send audio to device
  audio_write_lock.lock();

  qint64 actual_write = audio_io_device->write(reinterpret_cast<const char*>(&audio_ibuffer[offset]), max);

  if (actual_write > 0) {
    // average values and send to audio monitor
    int channels = audio_output->format().channelCount();
    qint64 lim = offset + (actual_write/sizeof(float));
    QVector<float> averages;
    averages.resize(channels);
    averages.fill(0.0);

    for (qint64 i=offset;i<lim;i++) {
      int channel = i%channels;
      averages[channel] = qMax(qAbs(audio_ibuffer[i]), averages[channel]);
    }

    panel_timeline.first()->audio_monitor->set_value(averages);
  }

  memset(&audio_ibuffer[offset], 0, actual_write);

  audio_ibuffer_read += (actual_write / sizeof(float));

  audio_write_lock.unlock();

  return actual_write;
}

double log_volume(double linear) {
  // expects a value between 0 and 1 (or more if amplifying)
  return (qExp(linear)-1.0f)/(M_E-1.0f);
}

void int32_to_char_array(qint32 i, char* array) {
  memcpy(array, &i, 4);
}

void write_wave_header(QFile& f, const QAudioFormat& format) {
  qint32 int32bit;
  char arr[4];

  // 4 byte riff header
  f.write("RIFF");

  // 4 byte file size, filled in later
  for (int i=0;i<4;i++) f.putChar(0);

  // 4 byte file type header + 4 byte format chunk marker
  f.write("WAVEfmt");
  f.putChar(0x20);

  // 4 byte length of the above format data (always 16 bytes)
  f.putChar(16);
  for (int i=0;i<3;i++) f.putChar(0);

  // 2 byte type format (1 is PCM)
  f.putChar(1);
  f.putChar(0);

  // 2 byte channel count
  int32bit = format.channelCount();
  int32_to_char_array(int32bit, arr);
  f.write(arr, 2);

  // 4 byte integer for sample rate
  int32bit = format.sampleRate();
  int32_to_char_array(int32bit, arr);
  f.write(arr, 4);

  // 4 byte integer for bytes per second
  int32bit = (format.sampleRate() * format.sampleSize() * format.channelCount()) / 8;
  int32_to_char_array(int32bit, arr);
  f.write(arr, 4);

  // 2 byte integer for bytes per sample per channel
  int32bit = (format.sampleSize() * format.channelCount()) / 8;
  int32_to_char_array(int32bit, arr);
  f.write(arr, 2);

  // 2 byte integer for bits per sample (16)
  int32bit = format.sampleSize();
  int32_to_char_array(int32bit, arr);
  f.write(arr, 2);

  // data chunk header
  f.write("data");

  // 4 byte integer for data chunk size (filled in later)?
  for (int i=0;i<4;i++) f.putChar(0);
}

void write_wave_trailer(QFile& f) {
  char arr[4];

  f.seek(4);

  // 4 bytes for total file size - 8 bytes
  qint32 file_size = qint32(f.size()) - 8;
  int32_to_char_array(file_size, arr);
  f.write(arr, 4);

  f.seek(40);

  // 4 bytes for data chunk size (file size - header)
  file_size = qint32(f.size()) - 44;
  int32_to_char_array(file_size, arr);
  f.write(arr, 4);
}

bool start_recording() {
  if (!olive::Global->CheckForActiveSequence(true)) {
    return false;
  }

  QString audio_path = QCoreApplication::translate("Audio", "%1 Audio").arg(olive::ActiveProjectFilename);
  QDir audio_dir(audio_path);
  if (!audio_dir.exists() && !audio_dir.mkpath(".")) {
    qCritical() << "Failed to create audio directory";
    return false;
  }

  QString audio_file_path;
  int file_number = 0;
  do {
    file_number++;

    QString audio_filename = QString("%1.wav").arg(
          QCoreApplication::translate("Audio", "Recording %1").arg(QString::number(file_number))
          );

    audio_file_path = audio_dir.filePath(audio_filename);
  } while (QFile(audio_file_path).exists());

  output_recording.setFileName(audio_file_path);
  if (!output_recording.open(QFile::WriteOnly)) {
    qCritical() << "Failed to open output file. Does Olive have permission to write to this directory?";
    return false;
  }

  QAudioFormat audio_format = audio_output->format();
  if (olive::config.recording_mode != audio_format.channelCount()) {
    audio_format.setChannelCount(olive::config.recording_mode);
  }

  QAudioDeviceInfo info = get_audio_device(QAudio::AudioInput);

  if (!info.isFormatSupported(audio_format)) {
    qWarning() << "Default format not supported, using nearest";
    audio_format = info.nearestFormat(audio_format);
  }
  write_wave_header(output_recording, audio_format);
  audio_input = new QAudioInput(info, audio_format);
  audio_input->start(&output_recording);
  recording = true;

  return true;
}

void stop_recording() {
  if (recording) {
    audio_input->stop();

    write_wave_trailer(output_recording);

    output_recording.close();

    delete audio_input;
    audio_input = nullptr;
    recording = false;
  }
}

QString get_recorded_audio_filename() {
  return output_recording.fileName();
}

void combobox_audio_sample_rates(QComboBox *combobox) {
  combobox->addItem("22050 Hz", 22050);
  combobox->addItem("24000 Hz", 24000);
  combobox->addItem("32000 Hz", 32000);
  combobox->addItem("44100 Hz", 44100);
  combobox->addItem("48000 Hz", 48000);
  combobox->addItem("88200 Hz", 88200);
  combobox->addItem("96000 Hz", 96000);
}

QObject* audio_wake_object = nullptr;
QMutex audio_wake_mutex;

QObject* GetAudioWakeObject()
{
  audio_wake_mutex.lock();

  QObject* wake_object = audio_wake_object;
  audio_wake_object = nullptr;

  audio_wake_mutex.unlock();

  return wake_object;
}

void SetAudioWakeObject(QObject *o)
{
  audio_wake_mutex.lock();
  audio_wake_object = o;
  audio_wake_mutex.unlock();
}

void WakeAudioWakeObject() {  
  QObject* audio_wake_object = GetAudioWakeObject();

  if (audio_wake_object != nullptr) {
    QMetaObject::invokeMethod(audio_wake_object, "play_wake", Qt::QueuedConnection);
  }
}
