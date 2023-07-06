/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef AUDIOCHANNELLAYOUT_H
#define AUDIOCHANNELLAYOUT_H

extern "C" {
#include <libavutil/channel_layout.h>
}

#include <QHash>
#include <QString>

#include "node/type.h"

namespace olive {

class AudioChannelLayout
{
public:
  static const AudioChannelLayout STEREO;

  AudioChannelLayout()
  {
    internal_ = {};
  }

  AudioChannelLayout(const AVChannelLayout &other) :
    AudioChannelLayout()
  {
    av_channel_layout_copy(&internal_, &other);
  }

  ~AudioChannelLayout()
  {
    av_channel_layout_uninit(&internal_);
  }

  AudioChannelLayout(const AudioChannelLayout& other) :
    AudioChannelLayout()
  {
    av_channel_layout_copy(&internal_, &other.internal_);
  }

  AudioChannelLayout& operator=(const AudioChannelLayout& other)
  {
    av_channel_layout_copy(&internal_, &other.internal_);
    return *this;
  }

  bool operator==(const AVChannelLayout &c) const
  {
    return !av_channel_layout_compare(&internal_, &c);
  }

  bool operator==(const AudioChannelLayout &c) const
  {
    return *this == c.internal_;
  }

  bool operator!=(const AudioChannelLayout &c) const { return !(*this == c); }
  bool operator!=(const AVChannelLayout &c) const { return !(*this == c); }

  bool isNull() const
  {
    return !av_channel_layout_check(&internal_);
  }

  QString toString() const
  {
    char buf[100];
    av_channel_layout_describe(&internal_, buf, sizeof(buf));
    return buf;
  }

  QString toHumanString() const
  {
    // TODO: Provide more usable strings than FFmpeg's descriptions
    return toString();
  }

  static AudioChannelLayout fromString(const QString &s)
  {
    AudioChannelLayout layout;
    av_channel_layout_from_string(&layout.internal_, s.toUtf8().constData());
    return layout;
  }

  static AudioChannelLayout fromMask(const uint64_t &i)
  {
    AudioChannelLayout layout;
    av_channel_layout_from_mask(&layout.internal_, i);
    return layout;
  }

  int count() const { return internal_.nb_channels; }

  void exportTo(AVChannelLayout &out) const
  {
    return exportTo(&out);
  }

  void exportTo(AVChannelLayout *out) const
  {
    av_channel_layout_copy(out, &internal_);
  }

  std::vector<type_t> getChannelNames() const;

  const AVChannelLayout *ref() const { return &internal_; }

private:
  AVChannelLayout internal_;

};

uint qHash(const AudioChannelLayout &l, uint seed = 0);

}

#endif // AUDIOCHANNELLAYOUT_H
