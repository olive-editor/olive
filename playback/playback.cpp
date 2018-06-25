#include "playback.h"

#include "project/clip.h"
#include "project/sequence.h"
#include "io/media.h"
#include "playback/audio.h"
#include "playback/cacher.h"
#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include <algorithm>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
}

#include <QObject>
#include <QOpenGLTexture>
#include <QDebug>
#include <QOpenGLPixelTransferOptions>

QList<Clip*> current_clips;
bool texture_failed = false;

QMutex cc_lock;

void handle_media(Sequence* sequence, long playhead, bool multithreaded) {
	for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);

		// if clip starts within one second and/or hasn't finished yet
        if (c != NULL) {
            if (is_clip_active(c, playhead)) {
                // if thread is already working, we don't want to touch this,
                // but we also don't want to hang the UI thread
                if (!c->open) {
                    if (c->open_lock.tryLock()) {
                        open_clip(c, multithreaded);

                        // add to current_clips, (insertion) sorted by track so composite them in order
                        cc_lock.lock();
                        bool found = false;
                        for (int j=0;j<current_clips.size();j++) {
                            if (current_clips[j]->track < c->track) {
                                current_clips.insert(current_clips.begin()+j, c);
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            current_clips.push_back(c);
                        }
                        cc_lock.unlock();
                    }
                }
            } else if (c->open) {
                close_clip(c);
            }
        }
	}
}

void open_clip(Clip* clip, bool multithreaded) {
	clip->multithreaded = multithreaded;
	if (multithreaded) {
		// maybe keep cacher instance in memory while clip exists for performance?
		clip->cacher = new Cacher(clip);
		QObject::connect(clip->cacher, SIGNAL(finished()), clip->cacher, SLOT(deleteLater()));

		clip->cacher->start(QThread::LowPriority);
	} else {
		open_clip_worker(clip);
		clip->lock.unlock();
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

void cache_clip(Clip* clip, long playhead, bool write_A, bool write_B, bool reset) {
	if (clip->multithreaded) {
		clip->cacher->playhead = playhead;
		clip->cacher->write_A = write_A;
		clip->cacher->write_B = write_B;
		clip->cacher->reset = reset;

		clip->can_cache.wakeAll();
	} else {
		cache_clip_worker(clip, playhead, write_A, write_B, reset);
	}
}

void get_clip_frame(Clip* c, long playhead) {
	if (c->open) {
		long clip_time = seconds_to_clip_frame(c, playhead_to_seconds(c, playhead));

		// do we need to update the texture?
		if ((!c->media_stream->infinite_length && c->texture_frame != clip_time) ||
				(c->media_stream->infinite_length && c->texture_frame == -1)) {
			AVFrame* current_frame = NULL;

			// get frame data
			if (c->media_stream->infinite_length) { // if clip is a still frame, we only need one
				if (c->cache_A.written) {
					// retrieve cached frame
					current_frame = c->cache_A.frames[0];
				} else if (c->multithreaded) {
					if (c->lock.tryLock()) {
						// grab image (multi-threaded)
						cache_clip(c, 0, false, false, true);
						c->lock.unlock();
					}
				} else {
					// grab image (single-threaded)
					reset_cache(c, playhead);
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
					if (c->cache_A.mutex.tryLock()) { // lock in case cacher is still writing to it
						using_cache_A = true;
						c->cache_A.unread = false;
						cache = c->cache_A.frames;
						cache_offset = c->cache_A.offset;
						c->cache_A.mutex.unlock();
					}
				} else if (c->cache_B.written && clip_time >= c->cache_B.offset && clip_time < c->cache_B.offset + c->cache_size) {
					if (c->cache_B.mutex.tryLock()) { // lock in case cacher is still writing to it
						using_cache_B = true;
						c->cache_B.unread = false;
						cache = c->cache_B.frames;
						cache_offset = c->cache_B.offset;
						c->cache_B.mutex.unlock();
					}
				} else {
					// this is technically bad, unless we just seeked
					c->cache_A.unread = c->cache_B.unread = false;
					cache_needs_reset = true;
				}

				if (cache != NULL) {
					current_frame = cache[clip_time - cache_offset];
				}

				// determine whether we should start filling the other cache
				if (!using_cache_A || !using_cache_B) {
					if (c->lock.tryLock()) {
						bool write_A = (!using_cache_A && !c->cache_A.unread);
						bool write_B = (!using_cache_B && !c->cache_B.unread);
						if (write_A || write_B) {
							long playhead;
							if (cache_needs_reset) {
								// if we have no cache and need to seek, start us at the current playhead...
								playhead = clip_time;
							} else {
								// ...otherwise start at the end of the current cache
								playhead = cache_offset + c->cache_size;
							}
							cache_clip(c, playhead, write_A, write_B, cache_needs_reset);
						}
						c->lock.unlock();
					}
				}
			}

			if (current_frame != NULL) {
				// set up opengl texture
				if (c->texture == NULL) {
					c->texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
					c->texture->setSize(c->media_stream->video_width, c->media_stream->video_height);
					c->texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
					c->texture->setMipLevels(c->texture->maximumMipLevels());
					c->texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
					c->texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
				}

				c->texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, current_frame->data[0]);
				c->texture_frame = clip_time;
			} else {
				texture_failed = true;
				qDebug() << "[ERROR] Failed to retrieve frame from cache (R:" << clip_time << "| A:" << c->cache_A.offset << "-" << c->cache_A.offset+c->cache_size-1 << "| B:" << c->cache_B.offset << "-" << c->cache_B.offset+c->cache_size-1 << "| WA:" << c->cache_A.written << "| WB:" << c->cache_B.written << ")";
			}
		}
	}
}

float playhead_to_seconds(Clip* c, long playhead) {
	// returns time in seconds
	return (std::max((long) 0, playhead - c->timeline_in) + c->clip_in)/c->sequence->frame_rate;
}

long seconds_to_clip_frame(Clip* c, float seconds) {
	// returns time as frame number (according to clip's frame rate)
	if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		return floor(seconds*av_q2d(av_guess_frame_rate(c->formatCtx, c->stream, c->frame)));
	} else {
		qDebug() << "[ERROR] seconds_to_clip_frame only works on video streams";
		return 0;
	}
}

float clip_frame_to_seconds(Clip* c, long clip_frame) {
	// returns frame number as seconds (decimal)
	if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		return (double) clip_frame / av_q2d(c->stream->avg_frame_rate);
	} else {
		qDebug() << "[ERROR] clip_frame_to_seconds only works on video streams";
		return 0;
	}
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
		} while (read_ret >= 0 && c->pkt->stream_index != c->media_stream->file_index);

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
                sws_scale(c->sws_ctx, c->frame->data, c->frame->linesize, 0, c->stream->codecpar->height, output->data, output->linesize);
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
}
