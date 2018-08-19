#include "playback.h"

#include "project/clip.h"
#include "project/sequence.h"
#include "io/media.h"
#include "playback/audio.h"
#include "playback/cacher.h"
#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "effects/effect.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
}

#include <algorithm>
#include <QObject>
#include <QOpenGLTexture>
#include <QDebug>
#include <QOpenGLPixelTransferOptions>

bool texture_failed = false;

void open_clip(Clip* clip, bool multithreaded) {
    if (multithreaded) {
        if (clip->open_lock.tryLock()) {
            clip->multithreaded = true;

            // maybe keep cacher instance in memory while clip exists for performance?
            clip->cacher = new Cacher(clip);
            QObject::connect(clip->cacher, SIGNAL(finished()), clip->cacher, SLOT(deleteLater()));

            clip->cacher->start(QThread::LowPriority);
        }
    } else {
        clip->multithreaded = false;
        clip->finished_opening = false;
        clip->open = true;

        open_clip_worker(clip);
    }
}

void close_clip(Clip* clip) {
	// destroy opengl texture in main thread
	if (clip->texture != NULL) {
		clip->texture->destroy();
		clip->texture = NULL;
	}

	if (clip->multithreaded) {
		clip->cacher->caching = false;
		clip->can_cache.wakeAll();
	} else {
		close_clip_worker(clip);
	}
}

void cache_clip(Clip* clip, long playhead, bool write_A, bool write_B, bool reset, Clip* nest) {
	if (clip->multithreaded) {
		clip->cacher->playhead = playhead;
		clip->cacher->write_A = write_A;
		clip->cacher->write_B = write_B;
		clip->cacher->reset = reset;
        clip->cacher->nest = nest;

		clip->can_cache.wakeAll();
	} else {
        cache_clip_worker(clip, playhead, write_A, write_B, reset, nest);
	}
}

bool get_clip_frame(Clip* c, long playhead) {
	if (c->open) {
		long sequence_clip_time = playhead - c->timeline_in + c->clip_in;
		long clip_time = refactor_frame_number(sequence_clip_time, c->sequence->frame_rate, av_q2d(av_guess_frame_rate(c->formatCtx, c->stream, c->frame)));

		// do we need to update the texture?
        MediaStream* ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->media_stream);
		if ((!ms->infinite_length/* && c->texture_frame != clip_time*/) ||
                (ms->infinite_length && c->texture_frame == -1)) {
			AVFrame* current_frame = NULL;
			bool no_frame = false;

			// get frame data
            if (ms->infinite_length) { // if clip is a still frame, we only need one
				if (c->cache_A.written) {
					// retrieve cached frame
					current_frame = c->cache_A.frames[0];
				} else if (c->lock.tryLock()) {
					// grab image
					cache_clip(c, 0, true, false, false, NULL);
					c->lock.unlock();
				}
			} else {
				// keeping a RAM cache improves performance, however it's detrimental when rendering
				// determine which cache contains the requested frame
				bool using_cache_A = false;
				bool using_cache_B = false;
				AVFrame** cache = NULL;
				long cache_offset = 0;
				bool cache_needs_reset = false;

				if (c->cache_A.written && clip_time >= c->cache_A.offset && clip_time < c->cache_A.offset + c->cache_size) {
					if (clip_time < c->cache_A.offset + c->cache_A.write_count) {
						if (c->cache_A.mutex.tryLock()) { // lock in case cacher is still writing to it
							using_cache_A = true;
							c->cache_A.unread = false;
							cache = c->cache_A.frames;
							cache_offset = c->cache_A.offset;
							c->cache_A.mutex.unlock();
						}
					} else {
						no_frame = true;
					}
				} else if (c->cache_B.written && clip_time >= c->cache_B.offset && clip_time < c->cache_B.offset + c->cache_size) {
					if (clip_time < c->cache_B.offset + c->cache_B.write_count) {
						if (c->cache_B.mutex.tryLock()) { // lock in case cacher is still writing to it
							using_cache_B = true;
							c->cache_B.unread = false;
							cache = c->cache_B.frames;
							cache_offset = c->cache_B.offset;
							c->cache_B.mutex.unlock();
						}
					} else {
						no_frame = true;
					}
				} else {
					// this is technically bad, unless we just seeked
					c->cache_A.unread = c->cache_B.unread = false;
					cache_needs_reset = true;
				}

				if (!no_frame) {
					if (cache != NULL) {
						current_frame = cache[clip_time - cache_offset];
					}

					// determine whether we should start filling the other cache
					if (!using_cache_A || !using_cache_B) {
						if (c->lock.tryLock()) {
							bool write_A = (!using_cache_A && !c->cache_A.unread);
							bool write_B = (!using_cache_B && !c->cache_B.unread);
							if (write_A || write_B) {
								// if we have no cache and need to seek, start us at the current playhead, otherwise start at the end of the current cache
								cache_clip(c, (cache_needs_reset) ? clip_time : cache_offset + c->cache_size, write_A, write_B, cache_needs_reset, NULL);
							}
							c->lock.unlock();
						}
					}
				}
			}

			if (current_frame != NULL) {
				// set up opengl texture
				if (c->texture == NULL) {
					c->texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
					c->texture->setSize(current_frame->width, current_frame->height);
					c->texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
					c->texture->setMipLevels(c->texture->maximumMipLevels());
					c->texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
					c->texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
				}

				memcpy(c->comp_frame, current_frame->data[0], c->comp_frame_size);
				QImage img(c->comp_frame, current_frame->width, current_frame->height, QImage::Format_RGBA8888);
				for (int i=0;i<c->effects.size();i++) {
					if (c->effects.at(i)->enable_image) {
						c->effects.at(i)->process_image(sequence_clip_time, img);
					}
				}

				c->texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, c->comp_frame);
				c->texture_frame = clip_time;

                return true;
			} else if (!no_frame) {
				texture_failed = true;
				qDebug() << "[ERROR] Failed to retrieve frame from cache (R:" << clip_time << "| A:" << c->cache_A.offset << "-" << c->cache_A.offset+c->cache_size-1 << "| B:" << c->cache_B.offset << "-" << c->cache_B.offset+c->cache_size-1 << "| WA:" << c->cache_A.written << "| WB:" << c->cache_B.written << ")";
			}
		}
	}
    return false;
}

double playhead_to_seconds(Clip* c, long playhead) {
	// returns time in seconds
    return (qMax((long) 0, playhead - c->timeline_in) + c->clip_in)/c->sequence->frame_rate;
}

long seconds_to_clip_frame(Clip* c, double seconds) {
	// returns time as frame number (according to clip's frame rate)
	if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		return floor(seconds*av_q2d(av_guess_frame_rate(c->formatCtx, c->stream, c->frame)));
	} else {
		qDebug() << "[ERROR] seconds_to_clip_frame only works on video streams";
		return 0;
	}
}

double clip_frame_to_seconds(Clip* c, long clip_frame) {
    // returns frame number in decimal seconds
	return (double) clip_frame / av_q2d(av_guess_frame_rate(c->formatCtx, c->stream, c->frame));
}

int retrieve_next_frame(Clip* c, AVFrame* f) {
	int result = 0;
    int receive_ret;

	// do we need to retrieve a new packet for a new frame?
    while ((receive_ret = avcodec_receive_frame(c->codecCtx, f)) == AVERROR(EAGAIN)) {
		int read_ret = 0;
		do {
			if (c->pkt_written) {
				av_packet_unref(c->pkt);
			}
			read_ret = av_read_frame(c->formatCtx, c->pkt);
			c->pkt_written = true;
        } while (read_ret >= 0 && c->pkt->stream_index != c->media_stream);

		if (read_ret >= 0) {
			int send_ret = avcodec_send_packet(c->codecCtx, c->pkt);
			if (send_ret < 0) {
				qDebug() << "[ERROR] Failed to send packet to decoder." << send_ret;
                return send_ret;
			}
        } else {
			if (read_ret != AVERROR_EOF) qDebug() << "[ERROR] Could not read frame." << read_ret;
			return read_ret; // skips trying to find a frame at all
		}
	}
	if (receive_ret < 0) {
		qDebug() << "[ERROR] Failed to receive packet from decoder." << receive_ret;
		result = receive_ret;
	}

	return result;
}

void retrieve_next_frame_raw_data(Clip* c, AVFrame* output) {
    if (c->reached_end) {
        qDebug() << "[WARNING] Attempted to retrieve frame of stream with no frames left";
    } else {
        int ret = retrieve_next_frame(c, c->frame);
        if (ret >= 0) {
            if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//				sws_scale(c->sws_ctx, c->frame->data, c->frame->linesize, 0, c->stream->codecpar->height, output->data, output->linesize);
//				output->pts = c->frame->best_effort_timestamp;
            } else if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                output->pts = c->frame->pts;
                ret = swr_convert_frame(c->swr_ctx, output, c->frame);
                if (ret < 0) {
                    qDebug() << "[ERROR] Failed to resample audio." << ret;
                }
            }
        } else if (ret == AVERROR_EOF) {
            c->reached_end = true;
        } else {
            qDebug() << "[WARNING] Raw frame data could not be retrieved." << ret;
        }
    }
}

bool is_clip_active(Clip* c, long playhead) {
	return c->timeline_in < playhead + ceil(c->sequence->frame_rate) && c->timeline_out > playhead && c->enabled;
}

void set_sequence(Sequence* s) {
    if (sequence != NULL) {
        // clean up - close all open clips
        for (int i=0;i<sequence->clip_count();i++) {
            Clip* c = sequence->get_clip(i);
            if (c != NULL && c->open) {
                close_clip(c);
            }
        }
    }
    sequence = s;
    panel_timeline->update_sequence();
    panel_viewer->update_sequence();
    panel_timeline->setFocus();
}
