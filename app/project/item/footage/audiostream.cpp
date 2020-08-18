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

#include "audiostream.h"

OLIVE_NAMESPACE_ENTER

AudioStream::AudioStream()
{
  set_type(kAudio);
}

QString AudioStream::description() const
{
  return QCoreApplication::translate("Stream", "%1: Audio - %2 Channels, %3Hz").arg(QString::number(index()),
                                                                                    QString::number(channels()),
                                                                                    QString::number(sample_rate()));
}

const int &AudioStream::channels() const
{
  return channels_;
}

void AudioStream::set_channels(const int &channels)
{
  channels_ = channels;
}

const uint64_t &AudioStream::channel_layout() const
{
  return layout_;
}

void AudioStream::set_channel_layout(const uint64_t &layout)
{
  layout_ = layout;
}

const int &AudioStream::sample_rate() const
{
  return sample_rate_;
}

void AudioStream::set_sample_rate(const int &sample_rate)
{
  sample_rate_ = sample_rate;
}

bool AudioStream::try_start_conforming(const AudioParams &params)
{
  QMutexLocker locker(proxy_access_lock());

  if (!currently_conforming_.contains(params)
      && !conformed_.contains(params)) {
    currently_conforming_.append(params);
    return true;
  }

  return false;
}

bool AudioStream::has_conformed_version(const AudioParams &params)
{
  QMutexLocker locker(proxy_access_lock());

  return conformed_.contains(params);
}

void AudioStream::append_conformed_version(const AudioParams &params)
{
  {
    QMutexLocker locker(proxy_access_lock());

    currently_conforming_.removeOne(params);
    conformed_.append(params);
  }

  emit ConformAppended(params);
}

OLIVE_NAMESPACE_EXIT
