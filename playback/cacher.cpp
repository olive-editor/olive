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
#include "debug.h"

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

#include <QOpenGLFramebufferObject>
#include <QtMath>
#include <math.h>

// temp debug shit
//#define AUDIOWARNINGS

int dest_format = AV_PIX_FMT_RGBA;

double bytes_to_seconds(int nb_bytes, int nb_channels, int sample_rate) {
	return ((double) (nb_bytes >> 1) / nb_channels / sample_rate);
}

void apply_audio_effects(Clip* c, double timecode_start, AVFrame* frame, int nb_bytes) {
	// perform all aud io effects
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
    long timeline_in = c->timeline_in;
    long timeline_out = c->timeline_out;
    if (nest != NULL) {
        timeline_in = refactor_frame_number(timeline_in, c->sequence->frame_rate, sequence->frame_rate) + nest->timeline_in;
        timeline_out = refactor_frame_number(timeline_out, c->sequence->frame_rate, sequence->frame_rate) + nest->timeline_in;
	}

	while (true) {
		AVFrame* frame;
		int nb_bytes = INT_MAX;

		switch (c->media_type) {
		case MEDIA_TYPE_FOOTAGE:
		{
			double timebase = av_q2d(c->stream->time_base);

			frame = c->queue.at(0);

			// retrieve frame
			bool new_frame = false;
			while ((c->frame_sample_index == -1 || c->frame_sample_index >= nb_bytes) && nb_bytes > 0) {
				// no more audio left in frame, get a new one
				if (!c->reached_end) {
					int loop = 0;

					if (c->reverse && !c->audio_just_reset) {
						avcodec_flush_buffers(c->codecCtx);
						c->reached_end = false;
						int64_t backtrack_seek = qMax(c->reverse_target - static_cast<int64_t>(av_q2d(av_inv_q(c->stream->time_base))), static_cast<int64_t>(0));
						av_seek_frame(c->formatCtx, c->stream->index, backtrack_seek, AVSEEK_FLAG_BACKWARD);
#ifdef AUDIOWARNINGS
						if (backtrack_seek == 0) {
							dout << "backtracked to 0";
						}
#endif
					}

					do {
						av_frame_unref(frame);

						int ret;

						while ((ret = av_buffersink_get_frame(c->buffersink_ctx, frame)) == AVERROR(EAGAIN)) {
							ret = retrieve_next_frame(c, c->frame);
							if (ret >= 0) {
								if ((ret = av_buffersrc_add_frame_flags(c->buffersrc_ctx, c->frame, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
									dout << "[ERROR] Could not feed filtergraph -" << ret;
									break;
								}
							} else {
								if (ret == AVERROR_EOF) {
#ifdef AUDIOWARNINGS
										dout << "reached EOF while reading";
#endif
									// TODO revise usage of reached_end in audio
									if (!c->reverse) {
										c->reached_end = true;
									} else {
									}
								} else {
									dout << "[WARNING] Raw audio frame data could not be retrieved." << ret;
									c->reached_end = true;
								}
								break;
							}
						}

						if (ret < 0) {
							if (ret != AVERROR_EOF) {
								dout << "[ERROR] Could not pull from filtergraph";
								c->reached_end = true;
								break;
							} else {
#ifdef AUDIOWARNINGS
								dout << "reached EOF while pulling from filtergraph";
#endif
								if (!c->reverse) break;
							}
						}

						if (c->reverse) {
							if (loop > 1) {
								AVFrame* rev_frame = c->queue.at(1);
								if (ret != AVERROR_EOF) {
									if (loop == 2) {
#ifdef AUDIOWARNINGS
										dout << "starting rev_frame";
#endif
										rev_frame->nb_samples = 0;
										rev_frame->pts = c->frame->pkt_pts;
									}
									int offset = rev_frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(rev_frame->format)) * rev_frame->channels;
#ifdef AUDIOWARNINGS
									dout << "offset 1:" << offset;
									dout << "retrieved samples:" << frame->nb_samples << "size:" << (frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels);
#endif
									memcpy(
											rev_frame->data[0]+offset,
											frame->data[0],
											(frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels)
										);
#ifdef AUDIOWARNINGS
									dout << "pts:" << c->frame->pts << "dur:" << c->frame->pkt_duration << "rev_target:" << c->reverse_target << "offset:" << offset << "limit:" << rev_frame->linesize[0];
#endif
								}

								rev_frame->nb_samples += frame->nb_samples;

//								if (c->frame->pts == c->rev_target) {
								if ((c->frame->pts >= c->reverse_target) || (ret == AVERROR_EOF)) {
									/*dout << "time for the end of rev cache" << rev_frame->nb_samples << c->rev_target << c->frame->pts << c->frame->pkt_duration << c->frame->nb_samples;
									dout << "diff:" << (c->frame->pkt_pts + c->frame->pkt_duration) - c->rev_target;
									int cutoff = qRound64 ((((c->frame->pkt_pts + c->frame->pkt_duration) - c->rev_target) * timebase) * c->sequence->audio_frequency);
									if (cutoff > 0) {
										dout << "cut off" << cutoff << "samples (rate:" << c->sequence->audio_frequency << ")";
										rev_frame->nb_samples -= cutoff;
									}*/

#ifdef AUDIOWARNINGS
									dout << "pre cutoff deets::: rev_frame.pts:" << rev_frame->pts << "rev_frame.nb_samples" << rev_frame->nb_samples << "rev_target:" << c->reverse_target;
#endif
									rev_frame->nb_samples = qRound64(static_cast<double>(c->reverse_target - rev_frame->pts) / c->stream->codecpar->sample_rate * c->sequence->audio_frequency);
#ifdef AUDIOWARNINGS
									dout << "post cutoff deets::" << rev_frame->nb_samples;
#endif

									int frame_size = rev_frame->nb_samples * rev_frame->channels * av_get_bytes_per_sample(static_cast<AVSampleFormat>(rev_frame->format));
									int half_frame_size = frame_size >> 1;

									int sample_size = rev_frame->channels*av_get_bytes_per_sample(static_cast<AVSampleFormat>(rev_frame->format));
									char* temp_chars = new char[sample_size];
									for (int i=0;i<half_frame_size;i+=sample_size) {
										for (int j=0;j<sample_size;j++) {
											temp_chars[j] = rev_frame->data[0][i+j];
										}
										for (int j=0;j<sample_size;j++) {
											rev_frame->data[0][i+j] = rev_frame->data[0][frame_size-i-sample_size+j];
										}
										for (int j=0;j<sample_size;j++) {
											rev_frame->data[0][frame_size-i-sample_size+j] = temp_chars[j];
										}
									}
									delete [] temp_chars;

									c->reverse_target = rev_frame->pts;
									frame = rev_frame;
									break;
								}
							}

							loop++;

#ifdef AUDIOWARNINGS
							dout << "loop" << loop;
#endif
						} else {
							frame->pts = c->frame->pts;
							break;
						}
					} while (true);
				} else {
					// if there is no more data in the file, we flush the remainder out of swresample
					break;
				}

				new_frame = true;

				if (c->frame_sample_index < 0) {
					c->frame_sample_index = 0;
				} else {
					c->frame_sample_index -= nb_bytes;
				}

				nb_bytes = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels;

                if (c->audio_just_reset) {
                    // get precise sample offset for the elected clip_in from this audio frame
					double target_sts = playhead_to_clip_seconds(c, c->audio_target_frame);
					double frame_sts = ((frame->pts - c->stream->start_time) * timebase);
					int nb_samples = qRound64((target_sts - frame_sts)*c->sequence->audio_frequency);
					c->frame_sample_index = nb_samples * 4;
#ifdef AUDIOWARNINGS
					dout << "fsts:" << frame_sts << "tsts:" << target_sts << "nbs:" << nb_samples << "nbb:" << nb_bytes << "rev_targetToSec:" << (c->reverse_target * timebase);
					dout << "fsi-calc:" << c->frame_sample_index;
#endif
					if (c->reverse) c->frame_sample_index = nb_bytes - c->frame_sample_index;
                    c->audio_just_reset = false;
				}

#ifdef AUDIOWARNINGS
				dout << "fsi-post-post:" << c->frame_sample_index;
#endif
				if (c->audio_buffer_write == 0) c->audio_buffer_write = get_buffer_offset_from_frame(c->sequence, qMax(timeline_in, c->audio_target_frame));

				int offset = (audio_ibuffer_read + 2048) - c->audio_buffer_write;
				if (offset > 0) {
					c->audio_buffer_write += offset;
					c->frame_sample_index += offset;
				}

				// try to correct negative fsi
				if (c->frame_sample_index < 0) {
					c->audio_buffer_write -= c->frame_sample_index;
					c->frame_sample_index = 0;
				}
			}

			if (c->reverse) frame = c->queue.at(1);

#ifdef AUDIOWARNINGS
			dout << "j" << c->frame_sample_index << nb_bytes;
#endif

			// apply any audio effects to the data
			if (nb_bytes == INT_MAX) nb_bytes = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels;
			if (new_frame) {
				apply_audio_effects(c, bytes_to_seconds(c->audio_buffer_write, 2, c->sequence->audio_frequency) + audio_ibuffer_timecode + ((double) (c->clip_in - c->timeline_in)/c->sequence->frame_rate), frame, nb_bytes);
			}
		}
			break;
		case MEDIA_TYPE_TONE:
			frame = c->frame;
			nb_bytes = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels;
			while ((c->frame_sample_index == -1 || c->frame_sample_index >= nb_bytes) && nb_bytes > 0) {
				// create "new frame"
				memset(c->frame->data[0], 0, nb_bytes);
				apply_audio_effects(c, bytes_to_seconds(frame->pts, frame->channels, frame->sample_rate), frame, nb_bytes);
				c->frame->pts += nb_bytes;
				c->frame_sample_index = 0;
				if (c->audio_buffer_write == 0) c->audio_buffer_write = get_buffer_offset_from_frame(c->sequence, qMax(timeline_in, c->audio_target_frame));
				int offset = audio_ibuffer_read - c->audio_buffer_write;
				if (offset > 0) {
					c->audio_buffer_write += offset;
					c->frame_sample_index += offset;
				}
			}
			break;
		default: // shouldn't ever get here
			dout << "[ERROR] Tried to cache a non-footage/tone clip";
			return;
		}

		// mix audio into internal buffer
		if (frame->nb_samples == 0) {
			break;
		} else {
			long buffer_timeline_out = get_buffer_offset_from_frame(c->sequence, timeline_out);
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
			}
#ifdef AUDIOWARNINGS
			if (c->audio_buffer_write >= buffer_timeline_out) dout << "timeline out at fsi" << c->frame_sample_index << "of frame ts" << c->frame->pts;
#endif
			audio_write_lock.unlock();

			if (c->frame_sample_index == nb_bytes) {
				c->frame_sample_index = -1;
			} else {
				// assume we have no more data to send
				break;
			}

//			dout << "ended" << c->frame_sample_index << nb_bytes;
		}
		if (c->reached_end) {
			frame->nb_samples = 0;
		}
	}
}

void cache_video_worker(Clip* c, long playhead) {
	int read_ret, send_ret, retr_ret;

	int64_t target_pts = seconds_to_timestamp(c, playhead_to_clip_seconds(c, playhead));

	int limit = c->max_queue_size;
	if (c->reverse) limit *= 2;
	/*if (c->reverse) {
		bool found = false;
		for (int i=0;i<c->queue.size();i++) {
			if (target_pts > c->queue.at(i)->pts && target_pts <= c->queue.at(i)->pts + c->queue.at(i)->pkt_duration) {
				found = true;
				break;
			}
		}
		if (!found) {
			// we need like one more frame
			limit = c->queue.size() + 1;
		}
	}*/

	if (c->queue.size() < limit) {
		int64_t eighth_second = av_q2d(av_inv_q(c->stream->time_base))*0.125;
		int64_t smallest_pts = INT64_MAX;
		if (c->reverse && c->queue.size() > 0) {
			int64_t quarter_sec = qRound64(av_q2d(av_inv_q(c->stream->time_base))) >> 2;
			for (int i=0;i<c->queue.size();i++) {
				smallest_pts = qMin(smallest_pts, c->queue.at(i)->pts);
			}
			avcodec_flush_buffers(c->codecCtx);
			c->reached_end = false;
			int64_t seek_ts = qMax(static_cast<int64_t>(0), smallest_pts - quarter_sec);
			av_seek_frame(c->formatCtx, c->stream->index, seek_ts, AVSEEK_FLAG_BACKWARD);
		} else {
			smallest_pts = target_pts;
		}

		while (true) {
			AVFrame* frame = av_frame_alloc();

			while ((retr_ret = av_buffersink_get_frame(c->buffersink_ctx, frame)) == AVERROR(EAGAIN)) {
				AVFrame* send_frame = c->frame;
				read_ret = (c->use_existing_frame) ? 0 : retrieve_next_frame(c, send_frame);
				c->use_existing_frame = false;
				if (read_ret >= 0) {
					bool send_it = false;

					if (c->reverse) {
						send_it = true;
					} else if (send_frame->pts > target_pts - eighth_second) {
						send_it = true;
					} else if (static_cast<Media*>(c->media)->get_stream_from_file_index(true, c->media_stream)->infinite_length) {
						send_it = true;
					} else {
						dout << "skipped adding a frame to the queue - fpts:" << send_frame->pts << "target:" << target_pts;
					}

					if (send_it) {
						if ((send_ret = av_buffersrc_add_frame_flags(c->buffersrc_ctx, send_frame, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
							dout << "[ERROR] Failed to add frame to buffer source." << send_ret;
							break;
						}
					}

					av_frame_unref(c->frame);
				} else {
					if (read_ret == AVERROR_EOF) {
						c->reached_end = true;
					} else {
						dout << "[ERROR] Failed to read frame." << read_ret;
					}
					break;
				}
			}

			if (retr_ret < 0) {
				if (retr_ret == AVERROR_EOF) {
					c->reached_end = true;
				} else {
					dout << "[ERROR] Failed to retrieve frame from buffersink." << retr_ret;
				}
				av_frame_free(&frame);
				break;
			} else {
				if (c->reverse && ((smallest_pts == target_pts && frame->pts >= smallest_pts) || (smallest_pts != target_pts && frame->pts > smallest_pts))) {
					av_frame_free(&frame);
					break;
				} else {
					// thread-safety while adding frame to the queue
					c->queue_lock.lock();
					c->queue.append(frame);

					if (!c->reverse && c->queue.size() == limit) {
//						if (rendering) {
							// see if we got the frame we needed (used for speed ups primarily)
							bool found = false;
							for (int i=0;i<c->queue.size();i++) {
								// TODO/NOTE: this will not work on clips that are sped up AND reversed
								if (c->queue.at(i)->pts >= target_pts) {
									found = true;
									break;
								}
							}
							if (found) {
								c->queue_lock.unlock();
								break;
							} else {
								// remove earliest frame and loop to store another
								c->queue_remove_earliest();
							}
//						} else {
//							break;
//						}
					}
					c->queue_lock.unlock();
				}
			}
		}
	}
}

void reset_cache(Clip* c, long target_frame) {
	// if we seek to a whole other place in the timeline, we'll need to reset the cache with new values	
	switch (c->media_type) {
	case MEDIA_TYPE_FOOTAGE:
	{
		MediaStream* ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->track < 0, c->media_stream);
		if (!ms->infinite_length) {
			if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				// clear current queue
				c->queue_clear();

				// seeks to nearest keyframe (target_frame represents internal clip frame)
				int64_t target_ts = seconds_to_timestamp(c, playhead_to_clip_seconds(c, target_frame));
				int64_t seek_ts = target_ts;
				int64_t timebase_half_second = qRound64(av_q2d(av_inv_q(c->stream->time_base)));
				if (c->reverse) seek_ts -= timebase_half_second;

				while (true) {
					// flush ffmpeg codecs
					avcodec_flush_buffers(c->codecCtx);
					c->reached_end = false;

					if (seek_ts > 0) {
						av_seek_frame(c->formatCtx, ms->file_index, seek_ts, AVSEEK_FLAG_BACKWARD);

						av_frame_unref(c->frame);
						int ret = retrieve_next_frame(c, c->frame);
						if (ret < 0) {
							dout << "[WARNING] Seeking terminated prematurely";
							break;
						}
						if (c->frame->pts <= target_ts) {
							c->use_existing_frame = true;
							break;
						} else {
							seek_ts -= timebase_half_second;
						}
					} else {
						av_frame_unref(c->frame);
						av_seek_frame(c->formatCtx, ms->file_index, 0, AVSEEK_FLAG_BACKWARD);
						c->use_existing_frame = false;
						break;
					}
				}
			} else if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
				// flush ffmpeg codecs
				avcodec_flush_buffers(c->codecCtx);
				c->reached_end = false;

				// seek (target_frame represents timeline timecode in frames, not clip timecode)

				int64_t timestamp = seconds_to_timestamp(c, playhead_to_clip_seconds(c, target_frame));

                if (c->reverse) {
					c->reverse_target = timestamp;
					timestamp -= av_q2d(av_inv_q(c->stream->time_base));
#ifdef AUDIOWARNINGS
					dout << "seeking to" << timestamp << "(originally" << c->reverse_target << ")";
				} else {
					dout << "reset called; seeking to" << timestamp;
#endif
				}
                av_seek_frame(c->formatCtx, ms->file_index, timestamp, AVSEEK_FLAG_BACKWARD);
				c->audio_target_frame = target_frame;
				c->frame_sample_index = -1;
				c->audio_just_reset = true;
			}
		}
	}
		break;
	case MEDIA_TYPE_TONE:
		c->reached_end = false;
		c->audio_target_frame = target_frame;
		c->frame_sample_index = -1;
		c->frame->pts = 0;
		break;
	}
}

Cacher::Cacher(Clip* c) : clip(c) {}

AVSampleFormat sample_format = AV_SAMPLE_FMT_S16;

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
			dout << "[ERROR] Could not open" << filename << "-" << err;
		}

		errCode = avformat_find_stream_info(clip->formatCtx, NULL);
		if (errCode < 0) {
			char err[1024];
			av_strerror(errCode, err, 1024);
			dout << "[ERROR] Could not open" << filename << "-" << err;
		}

		av_dump_format(clip->formatCtx, 0, filename, 0);

		clip->stream = clip->formatCtx->streams[ms->file_index];
		clip->codec = avcodec_find_decoder(clip->stream->codecpar->codec_id);
		clip->codecCtx = avcodec_alloc_context3(clip->codec);
		avcodec_parameters_to_context(clip->codecCtx, clip->stream->codecpar);

		clip->max_queue_size = (ms->infinite_length) ? 1 : qCeil(ms->video_frame_rate*0.5);
		if (ms->video_interlacing != VIDEO_PROGRESSIVE) clip->max_queue_size *= 2;

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
			dout << "[ERROR] Could not open codec";
		}

		// allocate filtergraph
		clip->filter_graph = avfilter_graph_alloc();
		if (clip->filter_graph == NULL) {
			dout << "[ERROR] Could not create filtergraph";
		}
		char filter_args[512];

		if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			/* SKIP_TYPE_SEEK is used if a video is playing at a speed so fast
			 * that it is quicker to seek to the next frame than to just play
			 * up to it (e.g. 2000% speed would require playing and skipping
			 * 20 frames per frame and it many cases it would be quicker to
			 * seek to it and cache in memory instead.
			 *
			 * TODO there could probably be a better heuristic than
			 * (speed >= 5) for using seek mode. Experiment with the value
			 * but also in the future perhaps we could implement a system
			 * of testing how long it takes to seek vs how long it takes to
			 * decode a frame and compare them to choose with method.
			 */
			clip->skip_type = (clip->speed < 5) ? SKIP_TYPE_DISCARD : SKIP_TYPE_SEEK;

			// create memory cache for video (deprecated)
			// clip->cache_size = (ms->infinite_length) ? 1 : ceil(av_q2d(clip->stream->avg_frame_rate)/4); // cache is half a second in total

			// if (clip->skip_type == SKIP_TYPE_SEEK) clip->cache_size *= 2;
			// if (ms->video_interlacing != VIDEO_PROGRESSIVE) clip->cache_size *= 2;

			snprintf(filter_args, sizeof(filter_args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
						clip->stream->codecpar->width,
						clip->stream->codecpar->height,
						clip->stream->codecpar->format,
						clip->stream->time_base.num,
						clip->stream->time_base.den,
						clip->stream->codecpar->sample_aspect_ratio.num,
						clip->stream->codecpar->sample_aspect_ratio.den
					 );

			avfilter_graph_create_filter(&clip->buffersrc_ctx, avfilter_get_by_name("buffer"), "in", filter_args, NULL, clip->filter_graph);
			avfilter_graph_create_filter(&clip->buffersink_ctx, avfilter_get_by_name("buffersink"), "out", NULL, NULL, clip->filter_graph);

            enum AVPixelFormat pix_fmts[] = { static_cast<AVPixelFormat>(dest_format), AV_PIX_FMT_NONE };
            if (av_opt_set_int_list(clip->buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) {
				dout << "[ERROR] Could not set output pixel format";
			}

			if (ms->video_interlacing == VIDEO_PROGRESSIVE) {
                avfilter_link(clip->buffersrc_ctx, 0, clip->buffersink_ctx, 0);
			} else {
				AVFilterContext* yadif_filter;
				char yadif_args[100];
				snprintf(yadif_args, sizeof(yadif_args), "mode=3:parity=%d", ((ms->video_interlacing == VIDEO_TOP_FIELD_FIRST) ? 0 : 1)); // try mode 1
				avfilter_graph_create_filter(&yadif_filter, avfilter_get_by_name("yadif"), "yadif", yadif_args, NULL, clip->filter_graph);

				avfilter_link(clip->buffersrc_ctx, 0, yadif_filter, 0);
				avfilter_link(yadif_filter, 0, clip->buffersink_ctx, 0);
			}

			avfilter_graph_config(clip->filter_graph, NULL);
		} else if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			if (clip->codecCtx->channel_layout == 0) clip->codecCtx->channel_layout = av_get_default_channel_layout(clip->stream->codecpar->channels);

			// set up cache
			clip->queue.append(av_frame_alloc());
			if (clip->reverse) {
				AVFrame* reverse_frame = av_frame_alloc();

				reverse_frame->format = sample_format;
				reverse_frame->nb_samples = clip->sequence->audio_frequency*2;
				reverse_frame->channel_layout = clip->sequence->audio_layout;
				reverse_frame->channels = av_get_channel_layout_nb_channels(clip->sequence->audio_layout);
				av_frame_get_buffer(reverse_frame, 0);

				clip->queue.append(reverse_frame);
			}

			snprintf(filter_args, sizeof(filter_args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
						clip->stream->time_base.num,
						clip->stream->time_base.den,
						clip->stream->codecpar->sample_rate,
						av_get_sample_fmt_name(clip->codecCtx->sample_fmt),
						clip->codecCtx->channel_layout
					 );

			avfilter_graph_create_filter(&clip->buffersrc_ctx, avfilter_get_by_name("abuffer"), "in", filter_args, NULL, clip->filter_graph);
			avfilter_graph_create_filter(&clip->buffersink_ctx, avfilter_get_by_name("abuffersink"), "out", NULL, NULL, clip->filter_graph);

			enum AVSampleFormat sample_fmts[] = { sample_format,  static_cast<AVSampleFormat>(-1) };
			if (av_opt_set_int_list(clip->buffersink_ctx, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN) < 0) {
				dout << "[ERROR] Could not set output sample format";
			}

			int64_t channel_layouts[] = { AV_CH_LAYOUT_STEREO, static_cast<AVSampleFormat>(-1) };
			if (av_opt_set_int_list(clip->buffersink_ctx, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN) < 0) {
				dout << "[ERROR] Could not set output sample format";
			}

			int target_sample_rate = clip->sequence->audio_frequency;

			if (qFuzzyCompare(clip->speed, 1.0)) {
				avfilter_link(clip->buffersrc_ctx, 0, clip->buffersink_ctx, 0);
			} else if (clip->maintain_audio_pitch) {
				char speed_param[10];
				snprintf(speed_param, sizeof(speed_param), "%f", clip->speed);

				AVFilterContext* tempo_filter;
				avfilter_graph_create_filter(&tempo_filter, avfilter_get_by_name("atempo"), "atempo", speed_param, NULL, clip->filter_graph);
				avfilter_link(clip->buffersrc_ctx, 0, tempo_filter, 0);
				avfilter_link(tempo_filter, 0, clip->buffersink_ctx, 0);
			} else {
				target_sample_rate = qRound64(clip->sequence->audio_frequency / clip->speed);
				avfilter_link(clip->buffersrc_ctx, 0, clip->buffersink_ctx, 0);
			}

			int sample_rates[] = { target_sample_rate, 0 };
			if (av_opt_set_int_list(clip->buffersink_ctx, "sample_rates", sample_rates, 0, AV_OPT_SEARCH_CHILDREN) < 0) {
				dout << "[ERROR] Could not set output sample rates";
			}

			avfilter_graph_config(clip->filter_graph, NULL);

			clip->audio_reset = true;
		}

		clip->frame = av_frame_alloc();
	}
		break;
	case MEDIA_TYPE_TONE:
		clip->frame = av_frame_alloc();
		clip->frame->format = sample_format;
		clip->frame->channel_layout = clip->sequence->audio_layout;
		clip->frame->channels = av_get_channel_layout_nb_channels(clip->frame->channel_layout);
		clip->frame->sample_rate = clip->sequence->audio_frequency;
		clip->frame->nb_samples = 2048;
		av_frame_make_writable(clip->frame);
		if (av_frame_get_buffer(clip->frame, 0)) {
			dout << "[ERROR] Could not allocate buffer for tone clip";
		}
		clip->audio_reset = true;
		break;
	}

	for (int i=0;i<clip->effects.size();i++) {
		clip->effects.at(i)->open();
	}

	clip->finished_opening = true;

	dout << "[INFO] Clip opened on track" << clip->track;
}

void cache_clip_worker(Clip* clip, long playhead, bool reset, Clip* nest) {	
	if (reset) {
		// note: for video, playhead is in "internal clip" frames - for audio, it's the timeline playhead
		reset_cache(clip, playhead);
		clip->audio_reset = false;
	}

	switch (clip->media_type) {
	case MEDIA_TYPE_FOOTAGE:
		if (clip->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			cache_video_worker(clip, playhead);
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
	clip->finished_opening = false;

	if (clip->media_type == MEDIA_TYPE_FOOTAGE) {
		clip->queue_clear();

		avfilter_graph_free(&clip->filter_graph);

		avcodec_close(clip->codecCtx);
		avcodec_free_context(&clip->codecCtx);
		avformat_close_input(&clip->formatCtx);
	}

	av_frame_free(&clip->frame);

    clip->reset();

	dout << "[INFO] Clip closed on track" << clip->track;
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
			cache_clip_worker(clip, playhead, reset, nest);
		}
	}

    close_clip_worker(clip);

    clip->lock.unlock();
	clip->open_lock.unlock();
}
