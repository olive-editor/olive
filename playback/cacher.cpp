#include "cacher.h"

#include "project/clip.h"
#include "project/sequence.h"
#include "io/media.h"
#include "playback/audio.h"
#include "playback/playback.h"
#include "effects/effect.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "effects/transition.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
}

#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QtMath>
#include <math.h>

int dest_format = AV_PIX_FMT_RGBA;

void apply_audio_effects(Clip* c, double timebase, AVFrame* frame, int nb_bytes) {
    // perform all audio effects
	double timecode_start, timecode_end;
	timecode_start = (frame->pts * timebase);
	timecode_end = timecode_start + ((double) (nb_bytes >> 1) / frame->channels / frame->sample_rate);

    for (int j=0;j<c->effects.size();j++) {
		Effect* e = c->effects.at(j);
		if (e->is_enabled()) e->process_audio(timecode_start, timecode_end, frame->data[0], nb_bytes, 2);
    }
	if (c->opening_transition != NULL) {
		if (c->media_type == MEDIA_TYPE_FOOTAGE) {
			double transition_start = (c->clip_in / c->sequence->frame_rate);
			double transition_end = transition_start + (c->opening_transition->length / c->sequence->frame_rate);
			if (timecode_end < transition_end) {
				double adjustment = transition_end - transition_start;
				double adjusted_range_start = (timecode_start - transition_start) / adjustment;
				double adjusted_range_end = (timecode_end - transition_start) / adjustment;
				c->opening_transition->process_audio(adjusted_range_start, adjusted_range_end, frame->data[0], nb_bytes, false);
			}
		}
	}
	if (c->closing_transition != NULL) {
		if (c->media_type == MEDIA_TYPE_FOOTAGE) {
			double transition_end = (c->clip_in + c->getLength()) / c->sequence->frame_rate;
			double transition_start = transition_end - (c->closing_transition->length / c->sequence->frame_rate);
			if (timecode_start > transition_start) {
				double adjustment = transition_end - transition_start;
				double adjusted_range_start = (timecode_start - transition_start) / adjustment;
				double adjusted_range_end = (timecode_end - transition_start) / adjustment;
				c->closing_transition->process_audio(adjusted_range_start, adjusted_range_end, frame->data[0], nb_bytes, true);
			}
		}
	}
}

void cache_audio_worker(Clip* c, Clip* nest) {
    int written = 0;
    int max_write = 16384;

    long timeline_in = c->timeline_in;
    long timeline_out = c->timeline_out;
    if (nest != NULL) {
        timeline_in = refactor_frame_number(timeline_in, c->sequence->frame_rate, sequence->frame_rate) + nest->timeline_in;
        timeline_out = refactor_frame_number(timeline_out, c->sequence->frame_rate, sequence->frame_rate) + nest->timeline_in;
	}

    while (written < max_write) {
        // gets one frame worth of audio and sends it to the audio buffer
        AVFrame* frame = c->cache_A.frames[0];

        if (c->need_new_audio_frame) {
            // no more audio left in frame, get a new one
            if (!c->reached_end) {
                retrieve_next_frame_raw_data(c, frame);
                int byte_count = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);
				double timebase = av_q2d(c->stream->time_base);
				apply_audio_effects(c, timebase, frame, byte_count);
				if (nest != NULL) apply_audio_effects(nest, timebase, frame, byte_count);
            } else {
                // set by retrieve_next_frame_raw_data indicating no more frames in file,
                // but there still may be samples in swresample
                swr_convert_frame(c->swr_ctx, frame, NULL);
            }

            c->need_new_audio_frame = false;
            if (c->audio_just_reset) {
                // get precise sample offset for the elected clip_in from this audio frame
				double target_sts = 0;
                if (c->audio_target_frame < timeline_in) {
                    target_sts = clip_frame_to_seconds(c, c->clip_in);
                } else {
                    target_sts = playhead_to_seconds(c, c->audio_target_frame);
                }
				double frame_sts = (frame->pts * av_q2d(c->stream->time_base));
                int nb_samples = qRound((target_sts - frame_sts)*c->sequence->audio_frequency);
                if (nb_samples == 0) {
                    c->frame_sample_index = 0;
                } else {
                    c->frame_sample_index = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(c->sequence->audio_layout), nb_samples, AV_SAMPLE_FMT_S16, 1);
                }
                c->audio_just_reset = false;
            } else {
                c->frame_sample_index = 0;
            }
        }

        if (frame->nb_samples == 0) {
            written = max_write;
        } else {
            int nb_bytes = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);

            int half_buffer = (audio_ibuffer_size/2);
            if (c->audio_buffer_write == 0) {
                c->audio_buffer_write = get_buffer_offset_from_frame(qMax(timeline_in, c->audio_target_frame));

				int offset = audio_ibuffer_read - c->audio_buffer_write;
				if (offset > 0) {
                    c->audio_buffer_write += offset;
                    c->frame_sample_index += offset;
				}
            }
            bool apply_effects = false;
            while (c->frame_sample_index > nb_bytes) {
				// get new frame
                retrieve_next_frame_raw_data(c, frame);
                apply_effects = true;
                nb_bytes = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);
                c->frame_sample_index -= nb_bytes;

                // code DISGUSTINGLY copy/pasted from above
                int offset = audio_ibuffer_read - c->audio_buffer_write;
				if (offset > 0) {
                    c->audio_buffer_write += offset;
                    c->frame_sample_index += offset;
                }
            }
            if (apply_effects) {
				double timebase = av_q2d(c->stream->time_base);
				apply_audio_effects(c, timebase, frame, nb_bytes);
				if (nest != NULL) apply_audio_effects(nest, timebase, frame, nb_bytes);
            }

			long buffer_timeline_out = get_buffer_offset_from_frame(timeline_out);
            while (c->frame_sample_index < nb_bytes) {
				if (c->audio_buffer_write >= audio_ibuffer_read+half_buffer || c->audio_buffer_write >= buffer_timeline_out) {
                    written = max_write;
                    break;
				} else {
					int upper_byte_index = (c->audio_buffer_write+1)%audio_ibuffer_size;
                    int lower_byte_index = (c->audio_buffer_write)%audio_ibuffer_size;
					qint16 old_sample = static_cast<qint16>((audio_ibuffer[upper_byte_index] & 0xFF) << 8 | (audio_ibuffer[lower_byte_index] & 0xFF));
					qint16 new_sample = static_cast<qint16>((frame->data[0][c->frame_sample_index+1] & 0xFF) << 8 | (frame->data[0][c->frame_sample_index] & 0xFF));
					qint32 mixed_sample = static_cast<qint32>(old_sample) + static_cast<qint32>(new_sample);
					mixed_sample = qMax(qMin(mixed_sample, static_cast<qint32>(INT16_MAX)), static_cast<qint32>(INT16_MIN));

					audio_ibuffer[upper_byte_index] = static_cast<quint8>((mixed_sample >> 8) & 0xFF);
					audio_ibuffer[lower_byte_index] = static_cast<quint8>(mixed_sample & 0xFF);

                    c->audio_buffer_write+=2;
                    c->frame_sample_index+=2;
					written+=2;
                }
            }
            if (c->frame_sample_index == nb_bytes) {
                c->need_new_audio_frame = true;
            }
        }
	}
}

void cache_video_worker(Clip* c, long playhead, ClipCache* cache) {
	cache->mutex.lock();

    cache->offset = playhead;

	bool error = false;

	int i = 0;

	if (!c->reached_end) {
		while (i < c->cache_size) {
			retrieve_next_frame_raw_data(c, cache->frames[i]);

			if (c->reached_end) break;
			i++;
		}
	}

	cache->write_count = i;

	if (!error) {
		// setting the cache to written even if it hasn't reached_end prevents playback from
		// signaling a seek/reset because it wasn't able to find the frame
		cache->written = true;
		cache->unread = true;
	}

	cache->mutex.unlock();
}

void reset_cache(Clip* c, long target_frame) {
	// if we seek to a whole other place in the timeline, we'll need to reset the cache with new values
    MediaStream* ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->track < 0, c->media_stream);
	if (!ms->infinite_length) {
		// flush ffmpeg codecs
		avcodec_flush_buffers(c->codecCtx);

		double timebase = av_q2d(c->stream->time_base);

		if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			// seeks to nearest keyframe (target_frame represents internal clip frame)

			av_seek_frame(c->formatCtx, ms->file_index, (int64_t) qFloor(clip_frame_to_seconds(c, target_frame) / timebase), AVSEEK_FLAG_BACKWARD);

			// play up to the frame we actually want
			long retrieved_frame = 0;
			AVFrame* temp = av_frame_alloc();
			do {
				retrieve_next_frame(c, temp);
				if (retrieved_frame == 0) {
					if (target_frame != 0) retrieved_frame = floor(temp->pts * timebase * av_q2d(av_guess_frame_rate(c->formatCtx, c->stream, temp)));
				} else {
					retrieved_frame++;
				}
			} while (retrieved_frame < target_frame);

			av_frame_free(&temp);
		} else if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			// seek (target_frame represents timeline timecode in frames, not clip timecode)
			swr_drop_output(c->swr_ctx, swr_get_out_samples(c->swr_ctx, 0));
            av_seek_frame(c->formatCtx, ms->file_index, playhead_to_seconds(c, target_frame) / timebase, AVSEEK_FLAG_BACKWARD);
            c->audio_target_frame = target_frame;
            c->need_new_audio_frame = true;
            c->audio_just_reset = true;
		}
	}
}

Cacher::Cacher(Clip* c) : clip(c) {}

void open_clip_worker(Clip* clip) {
	// opens file resource for FFmpeg and prepares Clip struct for playback
    Media* m = static_cast<Media*>(clip->media);
    QByteArray ba = m->url.toUtf8();
	const char* filename = ba.constData();
    MediaStream* ms = m->get_stream_from_file_index(clip->track < 0, clip->media_stream);

	int errCode = avformat_open_input(
			&clip->formatCtx,
			filename,
			NULL,
			NULL
		);
	if (errCode != 0) {
		char err[1024];
		av_strerror(errCode, err, 1024);
		qDebug() << "[ERROR] Could not open" << filename << "-" << err;
	}

	errCode = avformat_find_stream_info(clip->formatCtx, NULL);
	if (errCode < 0) {
		char err[1024];
		av_strerror(errCode, err, 1024);
		qDebug() << "[ERROR] Could not open" << filename << "-" << err;
	}

	av_dump_format(clip->formatCtx, 0, filename, 0);

    clip->stream = clip->formatCtx->streams[ms->file_index];
	clip->codec = avcodec_find_decoder(clip->stream->codecpar->codec_id);
	clip->codecCtx = avcodec_alloc_context3(clip->codec);
	avcodec_parameters_to_context(clip->codecCtx, clip->stream->codecpar);

	AVDictionary* opts = NULL;

	// decoding optimization configuration
	if (clip->stream->codecpar->codec_id != AV_CODEC_ID_PNG &&
		clip->stream->codecpar->codec_id != AV_CODEC_ID_APNG &&
		clip->stream->codecpar->codec_id != AV_CODEC_ID_TIFF &&
		clip->stream->codecpar->codec_id != AV_CODEC_ID_PSD) {
		av_dict_set(&opts, "threads", "auto", 0);
	}
	if (clip->stream->codecpar->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&opts, "tune", "fastdecode", 0);
		av_dict_set(&opts, "tune", "zerolatency", 0);
	}

	// Open codec
	if (avcodec_open2(clip->codecCtx, clip->codec, &opts) < 0) {
		qDebug() << "[ERROR] Could not open codec";
	}

	if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		// set up swscale context - primarily used for colorspace conversion
		// as "scaling" is actually done by OpenGL
		int dstW = ceil(clip->stream->codecpar->width/2)*2;
		int dstH = ceil(clip->stream->codecpar->height/2)*2;

		clip->sws_ctx = sws_getContext(
				clip->stream->codecpar->width,
				clip->stream->codecpar->height,
				static_cast<AVPixelFormat>(clip->stream->codecpar->format),
				dstW,
				dstH,
				static_cast<AVPixelFormat>(dest_format),
				SWS_FAST_BILINEAR,
				NULL,
				NULL,
				NULL
			);

		// create memory cache for video
		clip->cache_size = (ms->infinite_length) ? 1 : ceil(av_q2d(av_guess_frame_rate(clip->formatCtx, clip->stream, NULL))/4); // cache is half a second in total

		// infinite length doesn't need cache B
		clip->cache_A.frames = new AVFrame* [clip->cache_size];
		clip->cache_B.frames = new AVFrame* [clip->cache_size];
		for (int i=0;i<clip->cache_size;i++) {
			clip->cache_A.frames[i] = av_frame_alloc();
			av_frame_make_writable(clip->cache_A.frames[i]);
			clip->cache_A.frames[i]->width = dstW;
			clip->cache_A.frames[i]->height = dstH;
			clip->cache_A.frames[i]->format = dest_format;
			if (av_frame_get_buffer(clip->cache_A.frames[i], 0)) {
				qDebug() << "[ERROR] Could not allocate buffer for sws_frame";
			}
			clip->cache_A.frames[i]->linesize[0] = dstW*4;

			clip->cache_B.frames[i] = av_frame_alloc();
			av_frame_make_writable(clip->cache_B.frames[i]);
			clip->cache_B.frames[i]->width = dstW;
			clip->cache_B.frames[i]->height = dstH;
			clip->cache_B.frames[i]->format = dest_format;
			if (av_frame_get_buffer(clip->cache_B.frames[i], 0)) {
				qDebug() << "[ERROR] Could not allocate buffer for sws_frame";
			}
			clip->cache_B.frames[i]->linesize[0] = dstW*4;
		}
	} else if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
		// if FFmpeg can't pick up the channel layout (usually WAV), assume
		// based on channel count (doesn't support surround sound sources yet)
		if (clip->codecCtx->channel_layout == 0) {
            clip->codecCtx->channel_layout = guess_layout_from_channels(clip->stream->codecpar->channels);
		}

		int sample_format = AV_SAMPLE_FMT_S16;

		// init resampling context
		clip->swr_ctx = swr_alloc_set_opts(
				NULL,
                sequence->audio_layout,
				static_cast<AVSampleFormat>(sample_format),
                sequence->audio_frequency,
                clip->codecCtx->channel_layout,
				static_cast<AVSampleFormat>(clip->stream->codecpar->format),
				clip->stream->codecpar->sample_rate,
				0,
				NULL
			);
		swr_init(clip->swr_ctx);

		// set up cache
		clip->cache_A.frames = new AVFrame* [1];
		clip->cache_A.frames[0] = av_frame_alloc();
		clip->cache_A.frames[0]->format = sample_format;
        clip->cache_A.frames[0]->channel_layout = sequence->audio_layout;
		clip->cache_A.frames[0]->channels = av_get_channel_layout_nb_channels(clip->cache_A.frames[0]->channel_layout);
        clip->cache_A.frames[0]->sample_rate = sequence->audio_frequency;
		av_frame_make_writable(clip->cache_A.frames[0]);

        clip->audio_reset = true;
	}

	for (int i=0;i<clip->effects.size();i++) {
		clip->effects.at(i)->open();
	}

	clip->frame = av_frame_alloc();

    clip->finished_opening = true;

    qDebug() << "[INFO] Clip opened on track" << clip->track;
}

void cache_clip_worker(Clip* clip, long playhead, bool write_A, bool write_B, bool reset, Clip* nest) {
	if (reset) {
		// note: for video, playhead is in "internal clip" frames - for audio, it's the timeline playhead
		reset_cache(clip, playhead);
        clip->audio_reset = false;
	}

	if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		if (write_A) {
			cache_video_worker(clip, playhead, &clip->cache_A);
			playhead += clip->cache_size;
		}

		if (write_B) {
			cache_video_worker(clip, playhead, &clip->cache_B);
		}
    } else if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        cache_audio_worker(clip, nest);
	}
}

void close_clip_worker(Clip* clip) {
    // closes ffmpeg file handle and frees any memory used for caching
    MediaStream* ms = static_cast<Media*>(clip->media)->get_stream_from_file_index(clip->track < 0, clip->media_stream);
	if (clip->track < 0) {
		sws_freeContext(clip->sws_ctx);
	} else {
		swr_free(&clip->swr_ctx);
	}

	avcodec_close(clip->codecCtx);
	avcodec_free_context(&clip->codecCtx);
	avformat_close_input(&clip->formatCtx);

	for (int i=0;i<clip->cache_size;i++) {
		av_frame_free(&clip->cache_A.frames[i]);
        if (!ms->infinite_length) av_frame_free(&clip->cache_B.frames[i]);
	}
	delete [] clip->cache_A.frames;
    if (!ms->infinite_length) delete [] clip->cache_B.frames;

    av_frame_free(&clip->frame);

    clip->reset();

	qDebug() << "[INFO] Clip closed on track" << clip->track;
}

void Cacher::run() {
	// open_lock is used to prevent the clip from being destroyed before the cacher has closed it properly
    clip->lock.lock();
    clip->finished_opening = false;
    clip->open = true;
    caching = true;

	open_clip_worker(clip);

    while (caching) {
		clip->can_cache.wait(&clip->lock);
        if (!caching) {
			break;
		} else {
            cache_clip_worker(clip, playhead, write_A, write_B, reset, nest);
		}
	}

    close_clip_worker(clip);

    clip->lock.unlock();
	clip->open_lock.unlock();
}
