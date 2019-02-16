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

#ifndef CLIP_H
#define CLIP_H

#include <QWaitCondition>
#include <QMutex>
#include <QVector>

#include "marker.h"

class Cacher;
class Effect;
class Transition;
class QOpenGLFramebufferObject;
class ComboAction;
class Media;
struct Sequence;
struct Footage;
struct FootageStream;

struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SwrContext;
struct AVFilterGraph;
struct AVFilterContext;
struct AVDictionary;
class QOpenGLTexture;

class Clip {
public:
	Clip(Sequence* s);
	~Clip();
	Clip* copy(Sequence* s, bool duplicate_transitions = true);
	void reset_audio();
	void reset();
	void refresh();
	long get_clip_in_with_transition();
	long get_timeline_in_with_transition();
	long get_timeline_out_with_transition();
	long getLength();
	double getMediaFrameRate();
	long getMaximumLength();
	void recalculateMaxLength();
	int getWidth();
	int getHeight();
	void refactor_frame_rate(ComboAction* ca, double multiplier, bool change_timeline_points);
	Sequence* sequence;

	// queue functions
	void queue_clear();
	void queue_remove_earliest();

	// timeline variables (should be copied in copy())
	bool enabled;
	long clip_in;
	long timeline_in;
	long timeline_out;
	int track;
	QString name;
	quint8 color_r;
	quint8 color_g;
	quint8 color_b;
	Media* media;
	int media_stream;
	double speed;
	double cached_fr;
	bool reverse;
	bool maintain_audio_pitch;
	bool autoscale;

    // markers
    QVector<Marker>& get_markers();

	// other variables (should be deep copied/duplicated in copy())
	QList<Effect*> effects;
	QVector<int> linked;
	int opening_transition;
	Transition* get_opening_transition();
	int closing_transition;
	Transition* get_closing_transition();

	// media handling
	AVFormatContext* formatCtx;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecCtx;
	AVPacket* pkt;
	AVFrame* frame;
	AVDictionary* opts;
	long calculated_length;

	// temporary variables
	int load_id;
	bool undeletable;
	bool reached_end;
	bool pkt_written;
	bool open;
	bool finished_opening;
	bool replaced;
	bool ignore_reverse;
	int pix_fmt;

	// caching functions
	bool use_existing_frame;
	bool multithreaded;
	Cacher* cacher;
	QWaitCondition can_cache;
	int max_queue_size;
	QVector<AVFrame*> queue;
	QMutex queue_lock;
	QMutex lock;
	QMutex open_lock;
	int64_t last_invalid_ts;

	// converters/filters
	AVFilterGraph* filter_graph;
	AVFilterContext* buffersink_ctx;
	AVFilterContext* buffersrc_ctx;

	// video playback variables
	QOpenGLFramebufferObject** fbo;
	QOpenGLTexture* texture;
	long texture_frame;

	// audio playback variables
	int64_t reverse_target;
	int frame_sample_index;
	qint64 audio_buffer_write;
	bool audio_reset;
	bool audio_just_reset;
	long audio_target_frame;
private:
    QVector<Marker> markers;
};

#endif // CLIP_H
