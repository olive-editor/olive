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

#ifndef AUDIOHYBRIDDEVICE_H
#define AUDIOHYBRIDDEVICE_H

#include <QIODevice>

/**
 * @brief A device that can be connected to QAudioOutput and provides both "push" and "pull" functionality
 *
 * By default, a QAudioOutput works in either "push" or "pull" mode. Either it can automatically "pull" from a
 * QIODevice (reading from it whenever it requires samples), or we can "push" samples to it constantly (on a timer and
 * often in another thread) to make sure the buffer never underruns.
 *
 * This class functions as a "pull" device for the QAudioOutput, so the audio device automatically reads from it, but
 * then provides both "pull" and "push" functionality to the rest of the system. A QIODevice (like a raw PCM file)
 * can be connected and pulled from (good for continuous audio playback), and any amount of samples can also be pushed
 * (good for short bursts of sound, e.g. audio scrubbing).
 *
 * This does not support any "queuing", running ConnectDevice() or Push() will immediately discard anything that's
 * currently being sent to the device and replace it.
 */
class AudioHybridDevice : public QIODevice
{
  Q_OBJECT
public:
  AudioHybridDevice(QObject* parent = nullptr);

  void Push(const QByteArray &samples);

  /**
   * @brief Stop all audio output
   *
   * Whatever is being done (pulling from QIODevice or samples), it is stopped and cleared placing this into
   * "idle" state.
   *
   * Note that audio playback may not stop immediately after calling this function as the audio output may still have
   * samples in its buffer to output (QAudioOutput::stop() should be called as well for more immediate feedback). This
   * will prevent any further samples from being sent however.
   *
   * \see IsIdle()
   */
  void Stop();

  /**
   * @brief Connect a QIODevice (e.g. QFile) to start sending to the audio output
   *
   * This will clear any pushed samples or QIODevices currently being read and will start reading from this next time
   * the audio output requests data.
   */
  void ConnectDevice(QIODevice* device);

  /**
   * @brief Returns true if there are no more samples to be sent
   *
   * This is true if a device was connected or samples were pushed but we reached the end and no more data is available
   * to be sent.
   *
   * In this state, this device will continue returning an empty buffer (all zeroes) to the device to workaround
   * QAudioOutput's buffer underrun prevention with pushed samples. Therefore the QAudioOutput will never be put into
   * QAudio::IdleState. Therefore only way to determine whether the output is no longer receiving usable audio is to
   * check this function.
   */
  bool IsIdle();

  /**
   * @brief If enabled, this will emit the SentSamples() signal whenever samples are sent to the output device
   */
  void SetEnableSendingSamples(bool e);

signals:
  /**
   * @brief Signal emitted when this leaves "idle" state and has valid audio data that is ready to be sent
   */
  void HasSamples();

  /**
   * @brief Signal emitted when samples are sent to the output device
   *
   * This sends an array of values corresponding to the channel count. Each value is an average of all the samples just
   * sent to that channel. Useful for connecting to AudioMonitor.
   *
   * Can be disabled with SetEnableSendingSamples() to save CPU usage.
   */
  void SentSamples(QVector<double> averages);

protected:
  /**
   * @brief Internal QIODevice function for reading
   *
   * Returns either QIODevice::read() from the connected device, or samples from the pushed sample buffer if there is
   * none.
   *
   * If there is neither, this function returns a complete buffer of zeroes (i.e. silence). This is to allow
   * pushed samples that don't fulfill the QAudioOutput's internal buffer size. If it cannot fill its internal buffer
   * with the samples we send, it won't play anything until it receives more samples (enough to fill the buffer)
   * in an attempt to prevent buffer underrun).
   *
   * Since we don't care about buffer underrun with short bursts of sound, we return silent samples if the QAudioOutput
   * requests it. However this does mean the QAudioOutput never returns to QAudio::IdleState (since we never know if a
   * read is just to fill the "remainder" of a buffer - which needs zeroes - or just a general read - which technically
   * doesn't). It's recommended to check IsIdle() in tandem with the QAudioOutput::notify() signal to
   * determine whether the QAudioOutput is effectively idle and can be stopped.
   */
  virtual qint64 readData(char *data, qint64 maxSize) override;

  /**
   * @brief Internal QIODevice function for writing
   *
   * This class does not support writing, so this always returns -1 (QIODevice's documented error code for writing)
   */
  virtual qint64 writeData(const char *data, qint64 maxSize) override;

private:
  qint64 read_internal(char *data, qint64 maxSize);

  QIODevice* device_;

  QByteArray pushed_samples_;
  qint64 sample_index_;

  bool enable_sending_samples_;

};

#endif // AUDIOHYBRIDDEVICE_H
