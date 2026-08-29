/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include "internal.h"

FBPacket *fb_packet_alloc(void)
{
	AVPacket *avp = av_packet_alloc();
	if (!avp) {
		return nullptr;
	}

	FBPacket *p = new FBPacket;
	p->pkt = avp;
	return p;
}

void fb_packet_free(FBPacket **packet)
{
	if (packet && *packet) {
		av_packet_free(&(*packet)->pkt);
		delete *packet;
		*packet = nullptr;
	}
}

void fb_packet_unref(FBPacket *packet)
{
	if (packet && packet->pkt) {
		av_packet_unref(packet->pkt);
	}
}

int64_t fb_packet_get_pts(const FBPacket *packet)
{
	return packet ? packet->pkt->pts : FB_NOPTS_VALUE;
}

int64_t fb_packet_get_duration(const FBPacket *packet)
{
	return packet ? packet->pkt->duration : 0;
}

int fb_packet_get_size(const FBPacket *packet)
{
	return packet ? packet->pkt->size : 0;
}

const uint8_t *fb_packet_get_data(const FBPacket *packet)
{
	return packet ? packet->pkt->data : nullptr;
}

int fb_packet_get_stream_index(const FBPacket *packet)
{
	return packet ? packet->pkt->stream_index : -1;
}
