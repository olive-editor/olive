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

#include "audiorendererthreadbase.h"

#include <QDebug>

AudioRendererThreadBase::AudioRendererThreadBase(const int &sample_rate, const uint64_t &channel_layout, const olive::SampleFormat &format) :
  audio_params_(sample_rate, channel_layout, format)
{
}

AudioRendererParams *AudioRendererThreadBase::params()
{
  return &audio_params_;
}

void AudioRendererThreadBase::run()
{
  // Lock mutex for main loop
  mutex_.lock();

  // Signal that main thread can continue now
  WakeCaller();

  // Main loop (use Cancel() to exit it)
  ProcessLoop();

  // Unlock mutex before exiting
  mutex_.unlock();
}

void AudioRendererThreadBase::WakeCaller()
{
  // Signal that main thread can continue now
  caller_mutex_.lock();
  wait_cond_.wakeAll();
  caller_mutex_.unlock();
}

void AudioRendererThreadBase::StartThread(QThread::Priority priority)
{
  caller_mutex_.lock();

  // Start the thread
  QThread::start(priority);

  // Wait for thread to finish completion
  wait_cond_.wait(&caller_mutex_);

  caller_mutex_.unlock();
}
