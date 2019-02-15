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

#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <QVector>
#include <QMutex>

class Clip;
struct ClipCache;
struct Sequence;
struct AVFrame;

long refactor_frame_number(long framenumber, double source_frame_rate, double target_frame_rate);
bool clip_uses_cacher(Clip* clip);
void open_clip(Clip* clip, bool multithreaded);
void cache_clip(Clip* clip, long playhead, bool reset, bool scrubbing, QVector<Clip *> &nests, int playback_speed);
void close_clip(Clip* clip, bool wait);
void handle_media(Sequence* sequence, long playhead, bool multithreaded);
void reset_cache(Clip* c, long target_frame, int playback_speed);
void get_clip_frame(Clip* c, long playhead, bool &texture_failed);
double get_timecode(Clip* c, long playhead);

long playhead_to_clip_frame(Clip* c, long playhead);
double playhead_to_clip_seconds(Clip* c, long playhead);
int64_t seconds_to_timestamp(Clip* c, double seconds);
int64_t playhead_to_timestamp(Clip* c, long playhead);

int retrieve_next_frame(Clip* c, AVFrame* f);
bool is_clip_active(Clip* c, long playhead);
void get_next_audio(Clip* c, bool mix);
void set_sequence(Sequence* s);
void closeActiveClips(Sequence* s);

#endif // PLAYBACK_H
