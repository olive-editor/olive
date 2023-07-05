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

#include "render/audiochannellayout.h"

namespace olive {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
AudioChannelLayout const AudioChannelLayout::STEREO = AVChannelLayout(AV_CHANNEL_LAYOUT_STEREO);
#pragma GCC diagnostic pop

uint qHash(const AudioChannelLayout &l, uint seed)
{
  const AVChannelLayout *r = l.ref();

  uint h = ::qHash(r->order, seed);

  h ^= ::qHash(r->nb_channels, seed);

  if (r->order == AV_CHANNEL_ORDER_NATIVE || r->order == AV_CHANNEL_ORDER_AMBISONIC) {
    h ^= ::qHash(r->u.mask, seed);
  } else {
    h ^= ::qHashBits(r->u.map, sizeof(AVChannelCustom) * r->nb_channels, seed);
  }

  return h;
}

std::vector<type_t> AudioChannelLayout::getChannelNames() const
{
  std::vector<type_t> channels(internal_.nb_channels);

  char buf[8];
  for (int i = 0; i < internal_.nb_channels; i++) {
    AVChannel channel = av_channel_layout_channel_from_index(&internal_, i);
    av_channel_name(buf, sizeof(buf), channel);
    channels[i] = buf;
  }

  return channels;
}

}
