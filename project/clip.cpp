#include "clip.h"

#include "effects/effect.h"
#include "effects/transition.h"
#include "io/media.h"
#include "io/config.h"
#include "playback/playback.h"
#include "playback/cacher.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "panels/timeline.h"
#include "undo.h"

#include <QDebug>

extern "C" {
	#include <libavformat/avformat.h>
}

Clip::Clip(Sequence* s) :
    sequence(s),
    enabled(true),
    clip_in(0),
    timeline_in(0),
    timeline_out(0),
    track(0),
    undeletable(0),
    speed(1.0),
    media(NULL),
    opening_transition(NULL),
    closing_transition(NULL),
	pkt(new AVPacket()),
	replaced(false),
	filter_graph(NULL),
	texture(NULL),
	fbo(NULL),
	autoscale(config.autoscale_by_default),
	maintain_audio_pitch(false),
	reverse(false),
	use_existing_frame(false)
{
    reset();
}

Clip* Clip::copy(Sequence* s) {
    Clip* copy = new Clip(s);

    copy->enabled = enabled;
    copy->name = QString(name);
    copy->clip_in = clip_in;
    copy->timeline_in = timeline_in;
    copy->timeline_out = timeline_out;
    copy->track = track;
    copy->color_r = color_r;
    copy->color_g = color_g;
	copy->color_b = color_b;
    copy->media = media;
    copy->media_type = media_type;
    copy->media_stream = media_stream;
    copy->autoscale = autoscale;
	copy->speed = speed;
	copy->maintain_audio_pitch = maintain_audio_pitch;
	copy->reverse = reverse;

    for (int i=0;i<effects.size();i++) {
        copy->effects.append(effects.at(i)->copy(copy));
    }

    if (opening_transition != NULL) copy->opening_transition = opening_transition->copy();
    if (closing_transition != NULL) copy->closing_transition = closing_transition->copy();

	copy->recalculateMaxLength();

    return copy;
}

void Clip::reset() {
    audio_just_reset = false;
    open = false;
    finished_opening = false;
	pkt_written = false;
    reached_end = false;
    audio_reset = false;
	frame_sample_index = -1;
	audio_buffer_write = false;
	texture_frame = -1;
	formatCtx = NULL;
	stream = NULL;
	codec = NULL;
	codecCtx = NULL;
	texture = NULL;
}

void Clip::reset_audio() {
    switch (media_type) {
	case MEDIA_TYPE_FOOTAGE:
	case MEDIA_TYPE_TONE:
        audio_reset = true;
		frame_sample_index = -1;
        audio_buffer_write = 0;
		reached_end = false;
        break;
	case MEDIA_TYPE_SEQUENCE:
    {
        Sequence* nested_sequence = static_cast<Sequence*>(media);
        for (int i=0;i<nested_sequence->clips.size();i++) {
            Clip* c = nested_sequence->clips.at(i);
            if (c != NULL) c->reset_audio();
        }
    }
        break;
    }
}

void Clip::refresh() {
	// validates media if it was replaced
	if (replaced && media_type == MEDIA_TYPE_FOOTAGE) {
		Media* m = static_cast<Media*>(media);
		media_stream = (track < 0) ? m->video_tracks.at(0)->file_index : m->audio_tracks.at(0)->file_index;
	}
	replaced = false;

    // reinitializes all effects... just in case
    for (int i=0;i<effects.size();i++) {
		effects.at(i)->refresh();
	}

	recalculateMaxLength();
}

void Clip::clear_queue() {
	while (queue.size() > 0) {
		av_frame_free(&queue.first());
		queue.removeFirst();
	}
}

Clip::~Clip() {
    if (open) {
		close_clip(this);

        // make sure clip has closed before clip is destroyed
		if (multithreaded && media_type == MEDIA_TYPE_FOOTAGE) {
            cacher->wait();
        }
    }

    if (opening_transition != NULL) delete opening_transition;
    if (closing_transition != NULL) delete closing_transition;

    for (int i=0;i<effects.size();i++) {
        delete effects.at(i);
    }
	av_packet_unref(pkt);
	delete pkt;
}

long Clip::get_timeline_in_with_transition() {
    if (opening_transition != NULL && opening_transition->link != NULL) {
        return timeline_in - opening_transition->link->length;
    } else {
        return timeline_in;
    }
}

long Clip::get_timeline_out_with_transition() {
    if (closing_transition != NULL && closing_transition->link != NULL) {
        return timeline_out + closing_transition->link->length;
    } else {
        return timeline_out;
    }
}

// timeline functions
long Clip::getLength() {
    return timeline_out - timeline_in;
}

void Clip::recalculateMaxLength() {
	if (sequence != NULL) {
		double fr = this->sequence->frame_rate;

		fr /= speed;

		switch (media_type) {
		case MEDIA_TYPE_FOOTAGE:
		{
			Media* m = static_cast<Media*>(media);
			MediaStream* ms = m->get_stream_from_file_index(track < 0, media_stream);
			if (ms != NULL && ms->infinite_length) {
				calculated_length = LONG_MAX;
			} else {
				calculated_length = m->get_length_in_frames(fr);
			}
		}
			break;
		case MEDIA_TYPE_SEQUENCE:
		{
			Sequence* s = static_cast<Sequence*>(media);
			calculated_length = refactor_frame_number(s->getEndFrame(), s->frame_rate, fr);
		}
			break;
		/*case MEDIA_TYPE_SOLID:
		case MEDIA_TYPE_TONE:*/
		default:
			calculated_length = LONG_MAX;
			break;
		}
	}
}

long Clip::getMaximumLength() {
	return calculated_length;
}

double Clip::getMediaFrameRate() {
	Q_ASSERT(track < 0);
	switch (media_type) {
	case MEDIA_TYPE_FOOTAGE: return static_cast<Media*>(media)->get_stream_from_file_index(track < 0, media_stream)->video_frame_rate;
	case MEDIA_TYPE_SEQUENCE: return static_cast<Sequence*>(media)->frame_rate;
	}
	if (sequence != NULL) return sequence->frame_rate;
	return qSNaN();
}

int Clip::getWidth() {
	if (media == NULL && sequence != NULL) return sequence->width;
	switch (media_type) {
	case MEDIA_TYPE_FOOTAGE:
	{
		MediaStream* ms = static_cast<Media*>(media)->get_stream_from_file_index(track < 0, media_stream);
		if (ms != NULL) return ms->video_width;
        if (sequence != NULL) return sequence->width;
	}
	case MEDIA_TYPE_SEQUENCE:
	{
		Sequence* s = static_cast<Sequence*>(media);
		return s->width;
	}
	}
	return 0;
}

int Clip::getHeight() {
	if (media == NULL && sequence != NULL) return sequence->height;
	switch (media_type) {
	case MEDIA_TYPE_FOOTAGE:
	{
		MediaStream* ms = static_cast<Media*>(media)->get_stream_from_file_index(track < 0, media_stream);
		if (ms != NULL) return ms->video_height;
        if (sequence != NULL) return sequence->height;
	}
	case MEDIA_TYPE_SEQUENCE:
	{
		Sequence* s = static_cast<Sequence*>(media);
		return s->height;
	}
	}
	return 0;
}

void Clip::refactor_frame_rate(ComboAction* ca, double multiplier, bool change_timeline_points) {
	if (change_timeline_points) {
		ca->append(new MoveClipAction(this,
									  qRound((double) timeline_in * multiplier),
									  qRound((double) timeline_out * multiplier),
									  qRound((double) clip_in * multiplier),
								track));
	}

	for (int i=0;i<effects.size();i++) {
		Effect* e = effects.at(i);
		for (int j=0;j<e->row_count();j++) {
			EffectRow* r = e->row(j);
			for (int k=0;k<r->keyframe_times.size();k++) {
				long new_pos = r->keyframe_times.at(k) * multiplier;
				KeyframeMove* km = new KeyframeMove();
				km->movement = new_pos - r->keyframe_times.at(k);
				km->rows.append(r);
				km->keyframes.append(k);
				ca->append(km);
			}
		}
	}
}
