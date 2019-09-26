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

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <memory>
#include <QAudioOutput>
#include <QThread>

/**
 * @brief A thread for refreshing the total list of devices on the system
 *
 * Refreshing devices causes a noticeable pause in execution. Doing it another thread is intended to avoid this.
 */
class AudioRefreshDevicesThread : public QThread {
  Q_OBJECT
public:
  AudioRefreshDevicesThread();

  virtual void run() override;

  const QList<QAudioDeviceInfo>& input_devices();
  const QList<QAudioDeviceInfo>& output_devices();

signals:
  void ListsReady();

private:
  QList<QAudioDeviceInfo> input_devices_;
  QList<QAudioDeviceInfo> output_devices_;
};

class AudioManager : public QObject
{
  Q_OBJECT
public:
  static void CreateInstance();
  static void DestroyInstance();

  static AudioManager* instance();

  void RefreshDevices();

  bool IsRefreshing();

  const QList<QAudioDeviceInfo>& ListInputDevices();
  const QList<QAudioDeviceInfo>& ListOutputDevices();

signals:
  void DeviceListReady();

private:
  AudioManager();

  QList<QAudioDeviceInfo> input_devices_;

  QList<QAudioDeviceInfo> output_devices_;

  static AudioManager* audio_;

  std::unique_ptr<QAudioOutput> output_;

  bool refreshing_devices_;

  AudioRefreshDevicesThread refresh_thread_;

private slots:
  void RefreshThreadDone();

};

#endif // AUDIOMANAGER_H
