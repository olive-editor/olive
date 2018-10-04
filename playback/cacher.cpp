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
	#include <libavfilter/avfilter.h>
	#include <libavfilter/buffersrc.h>
	#include <libavfilter/buffersink.h>
	#include <libavutil/opt.h>
}

#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QtMath>
#include <math.h>

int dest_format = AV_PIX_FMT_RGBA;

double bytes_to_seconds(int nb_bytes, int nb_channels, int sample_rate) {
	return ((double) (nb_bytes >> 1) / nb_channels / sample_rate);
}

void apply_audio_effects(Clip* c, double timecode_start, AVFrame* frame, int nb_bytes) {
	// perform all audio effects
	double timecode_end;
	timecode_end = timecode_start + bytes_to_seconds(nb_bytes, frame->channels, frame->sample_rate);

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
		AVFrame* frame;
		int nb_bytes = INT_MAX;

		switch (c->media_type) {
		case MEDIA_TYPE_FOOTAGE:
		{
			frame = c->cache_A.frames[0];

			// retrieve frame
			bool new_frame = false;
			while ((c->frame_sample_index < 0 || c->frame_sample_index >= nb_bytes) && nb_bytes > 0) {
				// no more audio left in frame, get a new one
				if (!c->reached_end) {
					retrieve_next_frame_raw_data(c, frame);
				} else {
					// if there is no more data in the file, we flush the remainder out of swresample
					swr_convert_frame(c->swr_ctx, frame, NULL);
				}
				new_frame = true;

				if (c->frame_sample_index < 0) {
					c->frame_sample_index = 0;
				} else {
					c->frame_sample_index -= nb_bytes;
				}

				nb_bytes = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);

				if (c->audio_just_reset) {
					// get precise sample offset for the elected clip_in from this audio frame
					double target_sts = playhead_to_seconds(c, c->audio_target_frame);
					double frame_sts = (frame->pts * av_q2d(c->stream->time_base));
					int nb_samples = qRound((target_sts - frame_sts)*c->sequence->audio_frequency);
					c->frame_sample_index = (nb_samples == 0) ? 0 : av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(c->sequence->audio_layout), nb_samples, AV_SAMPLE_FMT_S16, 1);
					c->audio_just_reset = false;
				}

				if (c->audio_buffer_write == 0) c->audio_buffer_write = get_buffer_offset_from_frame(qMax(timeline_in, c->audio_target_frame));

				int offset = (audio_ibuffer_read + 2048) - c->audio_buffer_write;
				if (offset > 0) {
					c->audio_buffer_write += offset;
					c->frame_sample_index += offset;
				}
			}

			// apply any audio effects to the data
			if (nb_bytes == INT_MAX) nb_bytes = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);
			if (new_frame) {
				apply_audio_effects(c, bytes_to_seconds(c->audio_buffer_write, 2, sequence->audio_frequency) + audio_ibuffer_timecode, frame, nb_bytes);
			}
		}
			break;
		case MEDIA_TYPE_TONE:
			frame = c->frame;
			nb_bytes = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);
			if (c->frame_sample_index < 0 || c->frame_sample_index >= nb_bytes) {
				// create "new frame"
				memset(c->frame->data[0], 0, nb_bytes);
				apply_audio_effects(c, bytes_to_seconds(frame->pts, frame->channels, frame->sample_rate), frame, nb_bytes);
				c->frame->pts += nb_bytes;
				c->frame_sample_index = 0;
				if (c->audio_buffer_write == 0) c->audio_buffer_write = get_buffer_offset_from_frame(qMax(timeline_in, c->audio_target_frame));
				int offset = audio_ibuffer_read - c->audio_buffer_write;
				if (offset > 0) {
					c->audio_buffer_write += offset;
					c->frame_sample_index += offset;
				}
			}
			break;
		default: // shouldn't ever get here
			qDebug() << "[ERROR] Tried to cache a non-footage/tone clip";
			return;
		}

		// mix audio into internal buffer
		if (frame->nb_samples == 0) {
			break;
		} else {
			long buffer_timeline_out = get_buffer_offset_from_frame(timeline_out);
			audio_write_lock.lock();
			while (c->frame_sample_index < nb_bytes
				   && c->audio_buffer_write < audio_ibuffer_read+audio_ibuffer_size
				   && c->audio_buffer_write < buffer_timeline_out) {
				int upper_byte_index = (c->audio_buffer_write+1)%audio_ibuffer_size;
				int lower_byte_index = (c->audio_buffer_write)%audio_ibuffer_size;
				qint16 old_sample = static_cast<qint16>((audio_ibuffer[upper_byte_index] & 0xFF) << 8 | (audio_ibuffer[lower_byte_index] & 0xFF));
				qint16 new_sample = static_cast<qint16>((frame->data[0][c->frame_sample_index+1] & 0xFF) << 8 | (frame->data[0][c->frame_sample_index] & 0xFF));
				qint16 mixed_sample = mix_audio_sample(old_sample, new_sample);

				audio_ibuffer[upper_byte_index] = static_cast<quint8>((mixed_sample >> 8) & 0xFF);
				audio_ibuffer[lower_byte_index] = static_cast<quint8>(mixed_sample & 0xFF);

				c->audio_buffer_write+=2;
				c->frame_sample_index+=2;
				written+=2;
			}
			audio_write_lock.unlock();
			if (c->frame_sample_index == nb_bytes) {
				c->frame_sample_index = -1;
			} else {
				// assume we have no more data to send
				break;
			}
		}
	}
}

void cache_video_worker(Clip* c, long playhead, ClipCache* cache) {
	cache->mutex.lock();

    cache->offset = playhead;

	bool error = false;

	int i = 0;

//	double fr_ratio = c->sequence->frame_rate / c->frame_rate;

	/* old swscale solution
	if (!c->reached_end) {
		while (i < c->cache_size) {
			retrieve_next_frame_raw_data(c, cache->frames[i]);

			if (c->reached_end) break;
			i++;
		}
	}*/

	/* new AVFilter solution */
	if (!c->reached_end) {
		while (i < c->cache_size) {
			av_frame_unref(cache->frames[i]);

			int ret = (c->filter_graph == NULL) ? AVERROR(EAGAIN) : av_buffersink_get_frame(c->buffersink_ctx, cache->frames[i]);

			if (ret < 0) {
				if (ret == AVERROR(EAGAIN)) {
					ret = retrieve_next_frame(c, c->frame);

					if (ret >= 0) {
						// optimization: mathematically determine based on the sequence and clips' frame rates whether this frame will actually be shown
						// note: is not actually much faster and currently leads to crashes, may remove in the future
//						int proposed_frame = qFloor((i+cache->offset) * fr_ratio);

//						if (proposed_frame != c->last_cached_frame) {
							if ((ret = av_buffersrc_add_frame_flags(c->buffersrc_ctx, c->frame, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
								qDebug() << "[ERROR] Could not feed filtergraph -" << ret;
								error = true;
								break;
//							} else {
//								c->last_cached_frame = proposed_frame;
							}
//						} else {
//							i++;
//						}
					} else {
						if (ret == AVERROR_EOF) {
							c->reached_end = true;
						} else {
							qDebug() << "[WARNING] Raw frame data could not be retrieved." << ret;
							error = true;
						}
						break;
					}
				} else {
					if (ret != AVERROR_EOF) {
						qDebug() << "[ERROR] Could not pull from filtergraph";
						error = true;
					}
					break;
				}
			} else {
				i++;
			}
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
	switch (c->media_type) {
	case MEDIA_TYPE_FOOTAGE:
	{
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
						if (target_frame != 0) retrieved_frame = floor(temp->pts * timebase * av_q2d(c->stream->avg_frame_rate));
					} else {
						retrieved_frame++;
					}
				} while (retrieved_frame < target_frame);

				av_frame_free(&temp);

				c->last_cached_frame = -1;
			} else if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
				// seek (target_frame represents timeline timecode in frames, not clip timecode)
				swr_drop_output(c->swr_ctx, swr_get_out_samples(c->swr_ctx, 0));
				av_seek_frame(c->formatCtx, ms->file_index, playhead_to_seconds(c, target_frame) / timebase, AVSEEK_FLAG_BACKWARD);
				c->audio_target_frame = target_frame;
				c->frame_sample_index = -1;
				c->audio_just_reset = true;
			}
		}
	}
		break;
	case MEDIA_TYPE_TONE:
		c->audio_target_frame = target_frame;
		c->frame_sample_index = -1;
		c->frame->pts = 0;
		break;
	}
}

Cacher::Cacher(Clip* c) : clip(c) {}

int sample_format = AV_SAMPLE_FMT_S16;

void open_clip_worker(Clip* clip) {
	switch (clip->media_type) {
	case MEDIA_TYPE_FOOTAGE:
	{
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

		// optimized decoding settings
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

			//int dstW = ceil(clip->stream->codecpar->width/2)*2;
			//int dstH = ceil(clip->stream->codecpar->height/2)*2;

			/*clip->sws_ctx = sws_getContext(
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
				);*/

			// create memory cache for video
			clip->cache_size = (ms->infinite_length) ? 1 : ceil(av_q2d(clip->stream->avg_frame_rate)/4); // cache is half a second in total

			clip->cache_A.frames = new AVFrame* [clip->cache_size];
			clip->cache_B.frames = new AVFrame* [clip->cache_size];

			for (int i=0;i<clip->cache_size;i++) {
				clip->cache_A.frames[i] = av_frame_alloc();
				clip->cache_B.frames[i] = av_frame_alloc();
			}

			/*for (int i=0;i<clip->cache_size;i++) {
				clip->cache_A.frames[i] = av_frame_alloc();
				av_frame_make_writable(clip->cache_A.frames[i]);
				clip->cache_A.frames[i]->width = dstW;
				clip->cache_A.frames[i]->height = dstH;
				clip->cache_A.frames[i]->format = dest_format;
				if (av_frame_get_buffer(clip->cache_A.frames[i], 0)) {
					qDebug() << "[ERROR] Could not allocate buffer for sws_frame";
				}

				clip->cache_B.frames[i] = av_frame_alloc();
				av_frame_make_writable(clip->cache_B.frames[i]);
				clip->cache_B.frames[i]->width = dstW;
				clip->cache_B.frames[i]->height = dstH;
				clip->cache_B.frames[i]->format = dest_format;
				if (av_frame_get_buffer(clip->cache_B.frames[i], 0)) {
					qDebug() << "[ERROR] Could not allocate buffer for sws_frame";
				}
			}*/

			clip->filter_graph = avfilter_graph_alloc();
			if (clip->filter_graph == NULL) {
				qDebug() << "couldn't create filtergraph";
			}

			char args[512];
			snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
						clip->stream->codecpar->width,
						clip->stream->codecpar->height,
						clip->stream->codecpar->format,
						clip->stream->time_base.num,
						clip->stream->time_base.den,
						clip->stream->codecpar->sample_aspect_ratio.num,
						clip->stream->codecpar->sample_aspect_ratio.den
					 );

			avfilter_graph_create_filter(&clip->buffersrc_ctx, avfilter_get_by_name("buffer"), "in", args, NULL, clip->filter_graph);

			AVFilterContext* negate_ctx;
			avfilter_graph_create_filter(&negate_ctx, avfilter_get_by_name("setpts"), "rev", "0.5*PTS", NULL, clip->filter_graph);

			avfilter_graph_create_filter(&clip->buffersink_ctx, avfilter_get_by_name("buffersink"), "out", NULL, NULL, clip->filter_graph);

			enum AVPixelFormat pix_fmts[] = { static_cast<AVPixelFormat>(dest_format), AV_PIX_FMT_NONE };
			av_opt_set_int_list(clip->buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

			avfilter_link(clip->buffersrc_ctx, 0, negate_ctx, 0);
			avfilter_link(negate_ctx, 0, clip->buffersink_ctx, 0);

			avfilter_graph_config(clip->filter_graph, NULL);
		} else if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			// if FFmpeg can't pick up the channel layout (usually WAV), assume
			// based on channel count (doesn't support surround sound sources yet)
			if (clip->codecCtx->channel_layout == 0) {
				clip->codecCtx->channel_layout = av_get_default_channel_layout(clip->stream->codecpar->channels);
			}

			// init resampling context
			clip->swr_ctx = swr_alloc_set_opts(
					NULL,
					sequence->audio_layout,
					static_cast<AVSampleFormat>(sample_format),
					sequence->audio_frequency / clip->speed,
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
			clip->cache_A.frames[0]->sample_rate = sequence->audio_frequency / clip->speed;
			av_frame_make_writable(clip->cache_A.frames[0]);

			clip->audio_reset = true;
		}

		clip->frame = av_frame_alloc();
	}
		break;
	case MEDIA_TYPE_TONE:
		clip->frame = av_frame_alloc();
		clip->frame->format = sample_format;
		clip->frame->channel_layout = sequence->audio_layout;
		clip->frame->channels = av_get_channel_layout_nb_channels(clip->frame->channel_layout);
		clip->frame->sample_rate = sequence->audio_frequency;
		clip->frame->nb_samples = 2048;
		av_frame_make_writable(clip->frame);
		if (av_frame_get_buffer(clip->frame, 0)) {
			qDebug() << "[ERROR] Could not allocate buffer for tone clip";
		}
		clip->audio_reset = true;
		break;
	}

	for (int i=0;i<clip->effects.size();i++) {
		clip->effects.at(i)->open();
	}

	clip->finished_opening = true;

    qDebug() << "[INFO] Clip opened on track" << clip->track;
}

void cache_clip_worker(Clip* clip, long playhead, bool write_A, bool write_B, bool reset, Clip* nest) {
	if (reset) {
		// note: for video, playhead is in "internal clip" frames - for audio, it's the timeline playhead
		reset_cache(clip, playhead);
		clip->audio_reset = false;
	}

	switch (clip->media_type) {
	case MEDIA_TYPE_FOOTAGE:
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
		break;
	case MEDIA_TYPE_TONE:
		cache_audio_worker(clip, nest);
		break;
	}
}

void close_clip_worker(Clip* clip) {
	if (clip->media_type == MEDIA_TYPE_FOOTAGE) {
		// closes ffmpeg file handle and frees any memory used for caching
		MediaStream* ms = static_cast<Media*>(clip->media)->get_stream_from_file_index(clip->track < 0, clip->media_stream);
		if (clip->track < 0) {
//			sws_freeContext(clip->sws_ctx);
			avfilter_graph_free(&clip->filter_graph);
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
	}

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
