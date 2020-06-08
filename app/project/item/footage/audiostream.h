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

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <QVector>

#include "common/rational.h"
#include "render/audioparams.h"
#include "stream.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A Stream derivative containing audio-specific information
 */
class AudioStream : public Stream
{
  Q_OBJECT
public:
  AudioStream();

  virtual QString description() const override;

  const int& channels() const;
  void set_channels(const int& channels);

  const uint64_t& channel_layout() const;
  void set_channel_layout(const uint64_t& channel_layout);

  const int& sample_rate() const;
  void set_sample_rate(const int& sample_rate);

  bool try_start_conforming(const AudioParams& params);
  bool has_conformed_version(const AudioParams& params);
  void append_conformed_version(const AudioParams& params);

signals:
  void ConformAppended(OLIVE_NAMESPACE::AudioParams params);

private:
  int channels_;
  uint64_t layout_;
  int sample_rate_;

  QList<AudioParams> conformed_;

  QList<AudioParams> currently_conforming_;

};

using AudioStreamPtr = std::shared_ptr<AudioStream>;

OLIVE_NAMESPACE_EXIT

#endif // AUDIOSTREAM_H
