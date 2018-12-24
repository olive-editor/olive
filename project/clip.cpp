#include "clip.h"

#include "project/effect.h"
#include "project/transition.h"
#include "project/footage.h"
#include "io/config.h"
#include "playback/playback.h"
#include "playback/cacher.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "panels/timeline.h"
#include "project/media.h"
#include "undo.h"

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
    media(NULL),
    speed(1.0),
    reverse(false),
    maintain_audio_pitch(false),
    autoscale(config.autoscale_by_default),
    opening_transition(-1),
    closing_transition(-1),
    undeletable(false),
    replaced(false),
    ignore_reverse(false),
    use_existing_frame(false),
	filter_graph(NULL),
    fbo(NULL),
    opts(NULL)
{
	pkt = av_packet_alloc();
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
    copy->media_stream = media_stream;
    copy->autoscale = autoscale;
	copy->speed = speed;
	copy->maintain_audio_pitch = maintain_audio_pitch;
	copy->reverse = reverse;

    for (int i=0;i<effects.size();i++) {
        copy->effects.append(effects.at(i)->copy(copy));
    }

    // TODO make a replacemennt for this somehow
    if (get_opening_transition() != NULL && get_opening_transition()->secondary_clip == NULL) copy->opening_transition = get_opening_transition()->copy(copy, NULL);
    if (get_closing_transition() != NULL && get_closing_transition()->secondary_clip == NULL) copy->closing_transition = get_closing_transition()->copy(copy, NULL);

	copy->recalculateMaxLength();

    return copy;
}

void Clip::reset() {
    audio_just_reset = false;
    open = false;
    finished_opening = false;
	pkt_written = false;
    audio_reset = false;
	frame_sample_index = -1;
	audio_buffer_write = false;
	texture_frame = -1;
	formatCtx = NULL;
	stream = NULL;
	codec = NULL;
	codecCtx = NULL;
	texture = NULL;
    last_invalid_ts = -1;
}

void Clip::reset_audio() {
    if (media == NULL || media->get_type() == MEDIA_TYPE_FOOTAGE) {
        audio_reset = true;
        frame_sample_index = -1;
        audio_buffer_write = 0;
    } else if (media->get_type() == MEDIA_TYPE_SEQUENCE) {
        Sequence* nested_sequence = media->to_sequence();
        for (int i=0;i<nested_sequence->clips.size();i++) {
            Clip* c = nested_sequence->clips.at(i);
            if (c != NULL) c->reset_audio();
        }
    }
}

void Clip::refresh() {
	// validates media if it was replaced
    if (replaced && media != NULL && media->get_type() == MEDIA_TYPE_FOOTAGE) {
        Footage* m = media->to_footage();

        if (track < 0 && m->video_tracks.size() > 0)  {
            media_stream = m->video_tracks.at(0)->file_index;
        } else if (track >= 0 && m->audio_tracks.size() > 0) {
            media_stream = m->audio_tracks.at(0)->file_index;
        }
	}
	replaced = false;

    // reinitializes all effects... just in case
    for (int i=0;i<effects.size();i++) {
		effects.at(i)->refresh();
	}

    recalculateMaxLength();
}

void Clip::queue_clear() {
	while (queue.size() > 0) {
		av_frame_free(&queue.first());
		queue.removeFirst();
	}
}

void Clip::queue_remove_earliest() {
	int earliest_frame = 0;
	for (int i=1;i<queue.size();i++) {
		// TODO/NOTE: this will not work on sped up AND reversed video
		if (queue.at(i)->pts < queue.at(earliest_frame)->pts) {
			earliest_frame = i;
		}
	}
	av_frame_free(&queue[earliest_frame]);
    queue.removeAt(earliest_frame);
}

Transition* Clip::get_opening_transition() {
    if (opening_transition > -1) {
        return this->sequence->transitions.at(opening_transition);
    }
    return NULL;
}

Transition* Clip::get_closing_transition() {
    if (closing_transition > -1) {
        return this->sequence->transitions.at(closing_transition);
    }
    return NULL;
}

Clip::~Clip() {
    if (open) {
		close_clip(this);

        // make sure clip has closed before clip is destroyed
        if (multithreaded && media != NULL && media->get_type() == MEDIA_TYPE_FOOTAGE) {
            cacher->wait();
        }
    }

    if (opening_transition != -1) this->sequence->hard_delete_transition(this, TA_OPENING_TRANSITION);
    if (closing_transition != -1) this->sequence->hard_delete_transition(this, TA_CLOSING_TRANSITION);

    for (int i=0;i<effects.size();i++) {
        delete effects.at(i);
    }
	av_packet_free(&pkt);
}

long Clip::get_clip_in_with_transition() {
    if (get_opening_transition() != NULL && get_opening_transition()->secondary_clip != NULL) {
        // we must be the secondary clip, so return (timeline in - length)
        return clip_in - get_opening_transition()->get_true_length();
    }
    return clip_in;
}

long Clip::get_timeline_in_with_transition() {
    if (get_opening_transition() != NULL && get_opening_transition()->secondary_clip != NULL) {
        // we must be the secondary clip, so return (timeline in - length)
        return timeline_in - get_opening_transition()->get_true_length();
    }
    return timeline_in;
}

long Clip::get_timeline_out_with_transition() {
    if (get_closing_transition() != NULL && get_closing_transition()->secondary_clip != NULL) {
        // we must be the primary clip, so return (timeline out + length2)
        return timeline_out + get_closing_transition()->get_true_length();
    } else {
        return timeline_out;
    }
}

// timeline functions
long Clip::getLength() {
    return timeline_out - timeline_in;
}

double Clip::getMediaFrameRate() {
    Q_ASSERT(track < 0);
    if (media != NULL) {
        double rate = media->get_frame_rate(media_stream);
        if (!qIsNaN(rate)) return rate;
    }
    if (sequence != NULL) return sequence->frame_rate;
	return qSNaN();
}

void Clip::recalculateMaxLength() {
	if (sequence != NULL) {
		double fr = this->sequence->frame_rate;

		fr /= speed;

        calculated_length = LONG_MAX;

        if (media != NULL) {
            switch (media->get_type()) {
            case MEDIA_TYPE_FOOTAGE:
            {
                Footage* m = media->to_footage();
                FootageStream* ms = m->get_stream_from_file_index(track < 0, media_stream);
                if (ms != NULL && ms->infinite_length) {
                    calculated_length = LONG_MAX;
                } else {
                    calculated_length = m->get_length_in_frames(fr);
                }
            }
                break;
            case MEDIA_TYPE_SEQUENCE:
            {
                Sequence* s = media->to_sequence();
                calculated_length = refactor_frame_number(s->getEndFrame(), s->frame_rate, fr);
            }
                break;
            }
        }
	}
}

long Clip::getMaximumLength() {
	return calculated_length;
}

int Clip::getWidth() {
	if (media == NULL && sequence != NULL) return sequence->width;
    switch (media->get_type()) {
	case MEDIA_TYPE_FOOTAGE:
	{
        FootageStream* ms = media->to_footage()->get_stream_from_file_index(track < 0, media_stream);
		if (ms != NULL) return ms->video_width;
        if (sequence != NULL) return sequence->width;
	}
	case MEDIA_TYPE_SEQUENCE:
	{
        Sequence* s = media->to_sequence();
		return s->width;
	}
	}
	return 0;
}

int Clip::getHeight() {
	if (media == NULL && sequence != NULL) return sequence->height;
    switch (media->get_type()) {
	case MEDIA_TYPE_FOOTAGE:
	{
        FootageStream* ms = media->to_footage()->get_stream_from_file_index(track < 0, media_stream);
		if (ms != NULL) return ms->video_height;
        if (sequence != NULL) return sequence->height;
	}
	case MEDIA_TYPE_SEQUENCE:
	{
        Sequence* s = media->to_sequence();
		return s->height;
	}
	}
	return 0;
}

void Clip::refactor_frame_rate(ComboAction* ca, double multiplier, bool change_timeline_points) {
	if (change_timeline_points) {
        move_clip(ca, this,
                  qRound((double) timeline_in * multiplier),
                  qRound((double) timeline_out * multiplier),
                  qRound((double) clip_in * multiplier),
            track);
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
