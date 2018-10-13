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
#include "panels/effectcontrols.h"

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
#include <QOpenGLFramebufferObject>

bool texture_failed = false;

void open_clip(Clip* clip, bool multithreaded) {
	switch (clip->media_type) {
	case MEDIA_TYPE_FOOTAGE:
	case MEDIA_TYPE_TONE:
		clip->multithreaded = multithreaded;
		if (multithreaded) {
			if (clip->open_lock.tryLock()) {
				// maybe keep cacher instance in memory while clip exists for performance?
				clip->cacher = new Cacher(clip);
				QObject::connect(clip->cacher, SIGNAL(finished()), clip->cacher, SLOT(deleteLater()));
				clip->cacher->start((clip->track < 0) ? QThread::LowPriority : QThread::TimeCriticalPriority);
			}
		} else {
			clip->finished_opening = false;
			clip->open = true;

			open_clip_worker(clip);
		}
		break;
	case MEDIA_TYPE_SEQUENCE:
	case MEDIA_TYPE_SOLID:
		clip->open = true;
		break;
	}
}

void close_clip(Clip* clip) {
	// destroy opengl texture in main thread
	if (clip->texture != NULL) {
		delete clip->texture;
		clip->texture = NULL;
	}

	for (int i=0;i<clip->effects.size();i++) {
		clip->effects.at(i)->close();
	}

	if (clip->fbo != NULL) {
		delete clip->fbo[0];
		delete clip->fbo[1];
		delete [] clip->fbo;
		clip->fbo = NULL;
	}

	switch (clip->media_type) {
	case MEDIA_TYPE_FOOTAGE:
	case MEDIA_TYPE_TONE:
		if (clip->multithreaded) {
			clip->cacher->caching = false;
			clip->can_cache.wakeAll();
		} else {
			close_clip_worker(clip);
		}
		break;
	case MEDIA_TYPE_SEQUENCE:
		closeActiveClips(static_cast<Sequence*>(clip->media), false);
	case MEDIA_TYPE_SOLID:
		clip->open = false;
		break;
	}
}

void cache_clip(Clip* clip, long playhead, bool write_A, bool write_B, bool reset, Clip* nest) {
	if (clip->media_type == MEDIA_TYPE_FOOTAGE || clip->media_type == MEDIA_TYPE_TONE) {
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
}

bool get_clip_frame(Clip* c, long playhead) {
	if (c->finished_opening) {
		// do we need to update the texture?
		MediaStream* ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->track < 0, c->media_stream);

		long sequence_clip_time = qMax(0L, playhead - c->timeline_in + c->clip_in);

		if (c->reverse && !ms->infinite_length) {
			sequence_clip_time = c->getMaximumLength() - sequence_clip_time - 1;
		}

		double rate = c->getMediaFrameRate();
		if (c->skip_type == SKIP_TYPE_DISCARD) rate *= c->speed;
		long clip_time = refactor_frame_number(sequence_clip_time, c->sequence->frame_rate, rate);

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

			// TODO just removed a bunch of mutexes - is this safe????
			if (c->cache_A.written && clip_time >= c->cache_A.offset && clip_time < c->cache_A.offset + c->cache_size) {
				if (clip_time < (c->cache_A.offset + c->cache_A.write_count)) {
					using_cache_A = true;
					c->cache_A.unread = false;
					cache = c->cache_A.frames;
					cache_offset = c->cache_A.offset;
				} else {
					// frame is coming but isn't here yet, no need to reset cache
					no_frame = true;
				}
			} else if (c->cache_B.written && clip_time >= c->cache_B.offset && clip_time < c->cache_B.offset + c->cache_size) {
				if (clip_time < (c->cache_B.offset + c->cache_B.write_count)) {
					using_cache_B = true;
					c->cache_B.unread = false;
					cache = c->cache_B.frames;
					cache_offset = c->cache_B.offset;
				} else {
					// frame is coming but isn't here yet, no need to reset cache
					no_frame = true;
				}
			} else {
				// this is technically bad, unless we just seeked
				c->cache_A.write_count = 0;
				c->cache_B.write_count = 0;
				c->cache_A.unread = false;
				c->cache_B.unread = false;
				cache_needs_reset = true;
			}

			if (!no_frame) {
				if (cache != NULL) {
					current_frame = cache[clip_time - cache_offset];
				}

				// determine whether we should start filling the other cache
				if (!using_cache_A || !using_cache_B) {
					if (c->lock.tryLock()) {
						long cache_time;
						if (cache_needs_reset) {
							cache_time = (c->reverse) ? qMax(clip_time - c->cache_size, 0L) : qMax(clip_time, 0L);
						} else if (c->reverse) {
							cache_time = (cache_offset - c->cache_size);
						} else {
							cache_time = (cache_offset + c->cache_size);
						}

						bool write_A = (!using_cache_A && !c->cache_A.unread);
						bool write_B = (!using_cache_B && !c->cache_B.unread);
						if (write_A || write_B) {
							// if we have no cache and need to seek, start us at the current playhead, otherwise start at the end of the current cache
							cache_clip(c, cache_time, write_A, write_B, (cache_needs_reset || c->reverse), NULL);
						}
						c->lock.unlock();
					}
				}
			}
		}

		if (playhead >= c->timeline_in) {
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

				glPixelStorei(GL_UNPACK_ROW_LENGTH, current_frame->linesize[0]/4);
				c->texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, current_frame->data[0]);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				c->texture_frame = clip_time;
				return true;
			} else {
				texture_failed = true;
				qDebug() << "[ERROR] Failed to retrieve frame from cache (R:" << clip_time << "| A:" << c->cache_A.offset << "-" << c->cache_A.offset+c->cache_size-1 << "| B:" << c->cache_B.offset << "-" << c->cache_B.offset+c->cache_size-1 << "| WA:" << c->cache_A.written << "| WB:" << c->cache_B.written << ")";
			}
		}
	}
    return false;
}

double playhead_to_seconds(Clip* c, long playhead) {
	// returns time in seconds
	if (c->reverse) {
		return ((c->getMaximumLength() - (qMax(0L, playhead - c->timeline_in) + c->clip_in))/c->sequence->frame_rate)*c->speed;
	} else {
        return ((qMax(0L, playhead - c->timeline_in) + c->clip_in)/c->sequence->frame_rate)*c->speed;
    }
}

long seconds_to_clip_frame(Clip* c, double seconds) {
	// returns time as frame number (according to clip's frame rate)
	if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		return floor(seconds*c->getMediaFrameRate());
	} else {
		qDebug() << "[ERROR] seconds_to_clip_frame only works on video streams";
		return 0;
	}
}

double clip_frame_to_seconds(Clip* c, long clip_frame) {
    // returns frame number in decimal seconds
	return (double) clip_frame / c->getMediaFrameRate();
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
				c->pkt_written = false;
			}
			read_ret = av_read_frame(c->formatCtx, c->pkt);
			if (read_ret >= 0) {
				c->pkt_written = true;
			}
        } while (read_ret >= 0 && c->pkt->stream_index != c->media_stream);

		if (read_ret >= 0) {
			int send_ret = avcodec_send_packet(c->codecCtx, c->pkt);
			if (send_ret < 0) {
				qDebug() << "[ERROR] Failed to send packet to decoder." << send_ret;
                return send_ret;
			}
        } else {
			if (read_ret == AVERROR_EOF) {
				int send_ret = avcodec_send_packet(c->codecCtx, NULL);
				if (send_ret < 0) {
					qDebug() << "[ERROR] Failed to send packet to decoder." << send_ret;
					return send_ret;
				}
			} else {
				qDebug() << "[ERROR] Could not read frame." << read_ret;
				return read_ret; // skips trying to find a frame at all
			}
		}
	}
	if (receive_ret < 0) {
		qDebug() << "[ERROR] Failed to receive packet from decoder." << receive_ret;
		result = receive_ret;
	}

	return result;
}

bool is_clip_active(Clip* c, long playhead) {
    return c->enabled
            && c->timeline_in < playhead + ceil(c->sequence->frame_rate)
            && c->timeline_out > playhead
			&& playhead - c->timeline_in + c->clip_in < c->getMaximumLength();
}

void set_sequence(Sequence* s) {
	closeActiveClips(sequence, true);
	panel_effect_controls->clear_effects(true);
    sequence = s;
    panel_timeline->update_sequence();
	panel_sequence_viewer->update_media(MEDIA_TYPE_SEQUENCE, sequence);
    panel_timeline->setFocus();
}

void closeActiveClips(Sequence *s, bool wait) {
	if (s != NULL) {
        for (int i=0;i<s->clips.size();i++) {
            Clip* c = s->clips.at(i);
			if (c != NULL) {
				if (c->media_type == MEDIA_TYPE_SEQUENCE) {
					closeActiveClips(static_cast<Sequence*>(c->media), wait);
					close_clip(c);
				} else if (c->media_type == MEDIA_TYPE_FOOTAGE && c->open) {
					close_clip(c);
					if (c->multithreaded && wait) c->cacher->wait();
				}
			}
		}
	}
}
