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

#include "playback.h"

#include "project/clip.h"
#include "project/sequence.h"
#include "project/footage.h"
#include "playback/audio.h"
#include "playback/cacher.h"
#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "project/effect.h"
#include "panels/effectcontrols.h"
#include "project/media.h"
#include "io/config.h"
#include "io/proxygenerator.h"
#include "debug.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
}

#include <QtMath>
#include <QObject>
#include <QOpenGLTexture>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLFramebufferObject>
#include <QPainter>
#include <QCoreApplication>

#ifdef QT_DEBUG
//#define GCF_DEBUG
#endif

long refactor_frame_number(long framenumber, double source_frame_rate, double target_frame_rate) {
	return qRound((double(framenumber)/source_frame_rate)*target_frame_rate);
}

bool clip_uses_cacher(Clip* clip) {
	return (clip->media == nullptr && clip->track >= 0) || (clip->media != nullptr && clip->media->get_type() == MEDIA_TYPE_FOOTAGE);
}

void open_clip(Clip* clip, bool multithreaded) {
	if (clip_uses_cacher(clip)) {
		clip->multithreaded = multithreaded;
		if (multithreaded) {
			if (clip->open_lock.tryLock()) {
				// maybe keep cacher instance in memory while clip exists for performance?
				clip->cacher = new Cacher(clip);
				QObject::connect(clip->cacher, SIGNAL(finished()), clip->cacher, SLOT(deleteLater()));
				clip->cacher->start((clip->track < 0) ? QThread::HighPriority : QThread::TimeCriticalPriority);
			}
		} else {
			clip->finished_opening = false;
			clip->open = true;

			open_clip_worker(clip);
		}
	} else {
		clip->open = true;
		clip->finished_opening = true;
	}
}

void close_clip(Clip* clip, bool wait) {
	clip->finished_opening = false;

	// destroy opengl texture in main thread
	delete clip->texture;
	clip->texture = nullptr;

	for (int i=0;i<clip->effects.size();i++) {
		if (clip->effects.at(i)->is_open()) clip->effects.at(i)->close();
	}

	if (clip->fbo != nullptr) {
		// delete 3 fbos for nested sequences, 2 for most clips
		int fbo_count = (clip->media != nullptr && clip->media->get_type() == MEDIA_TYPE_SEQUENCE) ? 3 : 2;

		for (int j=0;j<fbo_count;j++) {
			delete clip->fbo[j];
		}

		delete [] clip->fbo;

		clip->fbo = nullptr;
	}

	if (clip_uses_cacher(clip)) {
		if (clip->multithreaded) {
			clip->cacher->caching = false;
			clip->can_cache.wakeAll();
			if (wait) {
				clip->open_lock.lock();
				clip->open_lock.unlock();
			}
		} else {
			close_clip_worker(clip);
		}
	} else {
		if (clip->media != nullptr && clip->media->get_type() == MEDIA_TYPE_SEQUENCE) {
			closeActiveClips(clip->media->to_sequence());
		}

		clip->open = false;
	}
}

void cache_clip(Clip* clip, long playhead, bool reset, bool scrubbing, QVector<Clip*>& nests, int playback_speed) {
	if (clip_uses_cacher(clip)) {
		if (clip->multithreaded) {
			clip->cacher->playhead = playhead;
			clip->cacher->reset = reset;
			clip->cacher->nests = nests;
			clip->cacher->scrubbing = scrubbing;
			clip->cacher->playback_speed = playback_speed;
			clip->cacher->queued = true;
			if (reset && clip->queue.size() > 0) clip->cacher->interrupt = true;

			clip->can_cache.wakeAll();
		} else {
			cache_clip_worker(clip, playhead, reset, scrubbing, nests, playback_speed);
		}
	}
}

double get_timecode(Clip* c, long playhead) {
	return ((double)(playhead-c->get_timeline_in_with_transition()+c->get_clip_in_with_transition())/(double)c->sequence->frame_rate);
}

void get_clip_frame(Clip* c, long playhead, bool& texture_failed) {
	if (c->finished_opening) {
		const FootageStream* ms = c->media->to_footage()->get_stream_from_file_index(c->track < 0, c->media_stream);

		int64_t target_pts = qMax(static_cast<int64_t>(0), playhead_to_timestamp(c, playhead));
		int64_t second_pts = qRound64(av_q2d(av_inv_q(c->stream->time_base)));
		if (ms->video_interlacing != VIDEO_PROGRESSIVE) {
			target_pts *= 2;
			second_pts *= 2;
		}

		AVFrame* target_frame = nullptr;

		bool reset = false;
		bool cache = true;

		c->queue_lock.lock();
		if (c->queue.size() > 0) {
			if (ms->infinite_length) {
				target_frame = c->queue.at(0);
#ifdef GCF_DEBUG
				dout << "GCF ==> USE PRECISE (INFINITE)";
#endif
			} else {
				// correct frame may be somewhere else in the queue
				int closest_frame = 0;

				for (int i=1;i<c->queue.size();i++) {
					//dout << "results for" << i << qAbs(c->queue.at(i)->pts - target_pts) << qAbs(c->queue.at(closest_frame)->pts - target_pts) << c->queue.at(i)->pts << target_pts;

					if (c->queue.at(i)->pts == target_pts) {
#ifdef GCF_DEBUG
						dout << "GCF ==> USE PRECISE";
#endif
						closest_frame = i;
						break;
					} else {
						if (c->queue.at(i)->pts > c->queue.at(closest_frame)->pts && c->queue.at(i)->pts < target_pts) {
							closest_frame = i;
						}
					}
				}

				// remove all frames earlier than this one from the queue
				target_frame = c->queue.at(closest_frame);
				int64_t next_pts = INT64_MAX;
				int64_t minimum_ts = target_frame->pts;

				int previous_frame_count = 0;
				if (olive::CurrentConfig.previous_queue_type == FRAME_QUEUE_TYPE_SECONDS) {
					minimum_ts -= (second_pts*olive::CurrentConfig.previous_queue_size);
				}

				//dout << "closest frame was" << closest_frame << "with" << target_frame->pts << "/" << target_pts;
				for (int i=0;i<c->queue.size();i++) {
					if (c->queue.at(i)->pts > target_frame->pts && c->queue.at(i)->pts < next_pts) {
						next_pts = c->queue.at(i)->pts;
					}
					if (c->queue.at(i) != target_frame && ((c->queue.at(i)->pts > minimum_ts) == c->reverse)) {
						if (olive::CurrentConfig.previous_queue_type == FRAME_QUEUE_TYPE_SECONDS) {
							//dout << "removed frame at" << i << "because its pts was" << c->queue.at(i)->pts << "compared to" << target_frame->pts;
							av_frame_free(&c->queue[i]); // may be a little heavy for the main thread?
							c->queue.removeAt(i);
							i--;
						} else {
							// TODO sort from largest to smallest
							previous_frame_count++;
						}
					}
				}

				if (olive::CurrentConfig.previous_queue_type == FRAME_QUEUE_TYPE_FRAMES) {
					while (previous_frame_count > qCeil(olive::CurrentConfig.previous_queue_size)) {
						int smallest = 0;
						for (int i=1;i<c->queue.size();i++) {
							if (c->queue.at(i)->pts < c->queue.at(smallest)->pts) {
								smallest = i;
							}
						}
						av_frame_free(&c->queue[smallest]);
						c->queue.removeAt(smallest);
						previous_frame_count--;
					}
				}

				if (next_pts == INT64_MAX) next_pts = target_frame->pts + target_frame->pkt_duration;

				// we didn't get the exact timestamp
				if (target_frame->pts != target_pts) {
					if (target_pts > target_frame->pts && target_pts <= next_pts) {
#ifdef GCF_DEBUG
						dout << "GCF ==> USE IMPRECISE";
#endif
					} else {
						int64_t pts_diff = qAbs(target_pts - target_frame->pts);
						if (c->reached_end && target_pts > target_frame->pts) {
#ifdef GCF_DEBUG
							dout << "GCF ==> EOF TOLERANT";
#endif
							c->reached_end = false;
							cache = false;
						} else if (target_pts != c->last_invalid_ts && (target_pts < target_frame->pts || pts_diff > second_pts)) {

#ifdef GCF_DEBUG
							dout << "GCF ==> RESET" << target_pts << "(" << target_frame->pts << "-" << target_frame->pts+target_frame->pkt_duration << ")";
#endif
							if (!olive::CurrentConfig.fast_seeking) target_frame = nullptr;
							reset = true;
							c->last_invalid_ts = target_pts;
						} else {
#ifdef GCF_DEBUG
							dout << "GCF ==> WAIT - target pts:" << target_pts << "closest frame:" << target_frame->pts;
#endif
							if (c->queue.size() >= c->max_queue_size) c->queue_remove_earliest();
							c->ignore_reverse = true;
							target_frame = nullptr;
						}
					}
				}
			}
		} else {
			reset = true;
		}

		if (target_frame == nullptr || reset) {
			// reset cache
			texture_failed = true;
			qInfo() << "Frame queue couldn't keep up - either the user seeked or the system is overloaded (queue size:" << c->queue.size() << ")";
		}

		if (target_frame != nullptr) {
			int nb_components = av_pix_fmt_desc_get(static_cast<enum AVPixelFormat>(c->pix_fmt))->nb_components;
			glPixelStorei(GL_UNPACK_ROW_LENGTH, target_frame->linesize[0]/nb_components);

			// 2 data buffers to ping-pong between
			bool using_db_1 = true;
			uint8_t* data_buffer_1 = target_frame->data[0];
			uint8_t* data_buffer_2 = nullptr;

			size_t frame_size = size_t(target_frame->linesize[0])*size_t(target_frame->height);

			// if proxy is currently being generated, show an on-screen message of its progress
			if (c->media->to_footage()->proxy
					&& c->media->to_footage()->proxy_path.isEmpty()) {
				// create buffers to draw on
				data_buffer_1 = new uint8_t[frame_size];
				data_buffer_2 = new uint8_t[frame_size];

				memcpy(data_buffer_1, target_frame->data[0], frame_size);

				// wrap data in a QImage for painting
				QImage img(data_buffer_1, target_frame->width, target_frame->height, QImage::Format_RGBA8888);

				// create QPainter process
				QPainter p(&img);

				// set font color to white
				p.setPen(Qt::white);

				// set font size relative to frame size (divided by 12)
				QFont overlay_font = p.font();
				overlay_font.setPixelSize(target_frame->height/12);
				p.setFont(overlay_font);

				// generate overlay text
				QString proxy_overlay_text = QCoreApplication::translate("Playback", "Generating Proxy: %1%").arg(proxy_generator.get_proxy_progress(c->media->to_footage()));

				int text_height = p.fontMetrics().descent() + p.fontMetrics().height();

				// draw semi-transparent black background
				p.fillRect(QRect(0,
								 target_frame->height-text_height,
								 p.fontMetrics().width(proxy_overlay_text),
								 text_height),
						   QColor(0, 0, 0, 128)
					);


				// draw text
				p.drawText(0,
						   target_frame->height-p.fontMetrics().descent(),
						   proxy_overlay_text);
			}

			for (int i=0;i<c->effects.size();i++) {
				Effect* e = c->effects.at(i);
				if (e->enable_image && e->is_enabled()) {
					if (data_buffer_1 == target_frame->data[0]) {
						data_buffer_1 = new uint8_t[frame_size];
						data_buffer_2 = new uint8_t[frame_size];

						memcpy(data_buffer_1, target_frame->data[0], frame_size);
					}
                    e->process_image(get_timecode(c, playhead),
                                     using_db_1 ? data_buffer_1 : data_buffer_2,
                                     using_db_1 ? data_buffer_2 : data_buffer_1,
                                     frame_size
                                 );
					using_db_1 = !using_db_1;
				}
			}

            c->texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, const_cast<const uint8_t*>(using_db_1 ? data_buffer_1 : data_buffer_2));

			if (data_buffer_1 != target_frame->data[0]) {
				delete [] data_buffer_1;
				delete [] data_buffer_2;
			}

			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		}

		c->queue_lock.unlock();

		// get more frames
		QVector<Clip*> empty;
		if (cache) cache_clip(c, playhead, reset, false, empty, false);
	}
}

long playhead_to_clip_frame(Clip* c, long playhead) {
	return (qMax(0L, playhead - c->get_timeline_in_with_transition()) + c->get_clip_in_with_transition());
}

double playhead_to_clip_seconds(Clip* c, long playhead) {
	// returns time in seconds
	long clip_frame = playhead_to_clip_frame(c, playhead);
	if (c->reverse) clip_frame = c->getMaximumLength() - clip_frame - 1;
	double secs = ((double) clip_frame/c->sequence->frame_rate)*c->speed;
	if (c->media != nullptr && c->media->get_type() == MEDIA_TYPE_FOOTAGE) secs *= c->media->to_footage()->speed;
	return secs;
}

int64_t seconds_to_timestamp(Clip* c, double seconds) {
	return qRound64(seconds * av_q2d(av_inv_q(c->stream->time_base))) + qMax((int64_t) 0, c->stream->start_time);
}

int64_t playhead_to_timestamp(Clip* c, long playhead) {
	return seconds_to_timestamp(c, playhead_to_clip_seconds(c, playhead));
}

int retrieve_next_frame(Clip* c, AVFrame* f) {
	int result = 0;
	int receive_ret;

	// do we need to retrieve a new packet for a new frame?
	av_frame_unref(f);
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
				qCritical() << "Failed to send packet to decoder." << send_ret;
				return send_ret;
			}
		} else {
			if (read_ret == AVERROR_EOF) {
				int send_ret = avcodec_send_packet(c->codecCtx, nullptr);
				if (send_ret < 0) {
					qCritical() << "Failed to send packet to decoder." << send_ret;
					return send_ret;
				}
			} else {
				qCritical() << "Could not read frame." << read_ret;
				return read_ret; // skips trying to find a frame at all
			}
		}
	}
	if (receive_ret < 0) {
		if (receive_ret != AVERROR_EOF) qCritical() << "Failed to receive packet from decoder." << receive_ret;
		result = receive_ret;
	}

	return result;
}

bool is_clip_active(Clip* c, long playhead) {
	// these buffers allow clips to be opened and prepared well before they're displayed
	// as well as closed a little after they're not needed anymore
	int open_buffer = qCeil(c->sequence->frame_rate*2);
	int close_buffer = qCeil(c->sequence->frame_rate);


	return c->enabled
			&& c->get_timeline_in_with_transition() < playhead + open_buffer
			&& c->get_timeline_out_with_transition() > playhead - close_buffer
			&& playhead - c->get_timeline_in_with_transition() + c->get_clip_in_with_transition() < c->getMaximumLength();
}

void set_sequence(Sequence* s) {
	panel_effect_controls->clear_effects(true);
	olive::ActiveSequence = s;
	panel_sequence_viewer->set_main_sequence();
	panel_timeline->update_sequence();
	panel_timeline->setFocus();
}

void closeActiveClips(Sequence *s) {
	if (s != nullptr) {
		for (int i=0;i<s->clips.size();i++) {
			Clip* c = s->clips.at(i);
			if (c != nullptr) {
				if (c->media != nullptr && c->media->get_type() == MEDIA_TYPE_SEQUENCE) {
					closeActiveClips(c->media->to_sequence());
					if (c->finished_opening) close_clip(c, true);
				} else if (c->finished_opening) {
					close_clip(c, true);
				}
			}
		}
	}
}
