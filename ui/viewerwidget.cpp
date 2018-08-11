#include "viewerwidget.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "project/effect.h"
#include "effects/transition.h"
#include "playback/playback.h"
#include "playback/audio.h"
#include "io/media.h"
#include "ui_timeline.h"

#include <QDebug>
#include <QPainter>
#include <QtMath>

extern "C" {
	#include <libavformat/avformat.h>
}

ViewerWidget::ViewerWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    multithreaded(true),
    force_audio(false),
    enable_paint(true),
    flip(false)
{
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	setFormat(format);

    // start audio sending thread
    audio_sender_thread.start();

    // error handler - retries after 250ms if we couldn't get the entire image
    retry_timer.setInterval(250);
	connect(&retry_timer, SIGNAL(timeout()), this, SLOT(retry()));
}

ViewerWidget::~ViewerWidget() {
    audio_sender_thread.close = true;
    audio_sender_thread.cond.wakeAll();
    audio_sender_thread.wait();
}

void ViewerWidget::deleteFunction() {
    // destroy all textures as well
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c->texture != NULL) {
            /* TODO NOTE: apparently "destroy()" is non-functional here because
             * it requires a valid current context, and I guess it's no longer
             * valid or current by the time this function is called. Not sure
             * how to handle that just yet...
             */

            c->texture->destroy();
            c->texture = NULL;
        }
    }
}

void ViewerWidget::retry() {
	update();
}

void ViewerWidget::initializeGL() {
    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(deleteFunction()));

    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);
    glMatrixMode(GL_PROJECTION);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    update();
}

//void ViewerWidget::resizeGL(int w, int h)
//{
//}

void ViewerWidget::paintEvent(QPaintEvent *e) {
    if (enable_paint) QOpenGLWidget::paintEvent(e);
}

void ViewerWidget::compose_sequence(Clip* nest, bool render_audio) {
    Sequence* s = sequence;
	long playhead = panel_timeline->playhead;

    if (nest != NULL) {
        s = static_cast<Sequence*>(nest->media);
        playhead += nest->clip_in - nest->timeline_in;
        playhead = refactor_frame_number(playhead, sequence->frame_rate, s->frame_rate);
    }

    QVector<Clip*> current_clips;

    for (int i=0;i<s->clip_count();i++) {
        Clip* c = s->get_clip(i);

        // if clip starts within one second and/or hasn't finished yet
		if (c != NULL && !(nest != NULL && !same_sign(c->track, nest->track))) {
            bool clip_is_active = false;

            switch (c->media_type) {
            case MEDIA_TYPE_FOOTAGE:
            {
                Media* m = static_cast<Media*>(c->media);
				if (m->ready) {
					if (m->get_stream_from_file_index(c->media_stream) != NULL
							&& is_clip_active(c, playhead)) {
						// if thread is already working, we don't want to touch this,
						// but we also don't want to hang the UI thread
						if (!c->open) {
							open_clip(c, multithreaded);
						}

						clip_is_active = true;
					} else if (c->open) {
						close_clip(c);
					}
				} else {
					texture_failed = true;
				}
            }
                break;
            case MEDIA_TYPE_SEQUENCE:
                clip_is_active = true;
                break;
            }

            if (clip_is_active) {
                bool added = false;
                for (int j=0;j<current_clips.size();j++) {
                    if (current_clips.at(j)->track < c->track) {
                        current_clips.insert(j, c);
                        added = true;
                        break;
                    }
                }
                if (!added) {
                    current_clips.append(c);
                }
            }
        }
    }

    for (int i=0;i<current_clips.size();i++) {
        Clip* c = current_clips.at(i);

        if (c->media_type == MEDIA_TYPE_FOOTAGE && !c->finished_opening) {
            qDebug() << "[WARNING] Tried to display clip" << i << "but it's closed";
            texture_failed = true;
        } else {
            switch (c->media_type) {
            case MEDIA_TYPE_FOOTAGE:
				qDebug() << "clip";
                if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    // start preparing cache
                    get_clip_frame(c, playhead);

                    if (c->texture == NULL) {
                        qDebug() << "[WARNING] Texture hasn't been created yet";
                        texture_failed = true;
                    } else if (playhead >= c->timeline_in) {
                        MediaStream* ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->media_stream);

                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glColor4f(1.0, 1.0, 1.0, 1.0);
                        glLoadIdentity();

                        int half_width = sequence->width/2;
                        int half_height = sequence->height/2;
                        if (flip) half_height = -half_height;
                        glOrtho(-half_width, half_width, half_height, -half_height, -1, 1);
                        int anchor_x = ms->video_width/2;
                        int anchor_y = ms->video_height/2;

                        // perform all transform effects
                        c->run_video_pre_effect_stack(playhead, &anchor_x, &anchor_y);

                        if (nest != NULL) {
                            nest->run_video_pre_effect_stack(playhead, &anchor_x, &anchor_y);
                        }

                        int anchor_right = ms->video_width - anchor_x;
                        int anchor_bottom = ms->video_height - anchor_y;

                        c->texture->bind();

                        glBegin(GL_QUADS);
                        glTexCoord2f(0.0, 0.0);
                        glVertex2f(-anchor_x, -anchor_y);
                        glTexCoord2f(1.0, 0.0);
                        glVertex2f(anchor_right, -anchor_y);
                        glTexCoord2f(1.0, 1.0);
                        glVertex2f(anchor_right, anchor_bottom);
                        glTexCoord2f(0.0, 1.0);
                        glVertex2f(-anchor_x, anchor_bottom);
                        glEnd();

                        c->texture->release();

                        // perform all transform effects
                        c->run_video_post_effect_stack();
                    }
                } else if (render_audio &&
                           c->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
                           c->lock.tryLock()) {
                    // clip is not caching, start caching audio
                    cache_clip(c, playhead, false, false, c->audio_reset, nest);
                    c->lock.unlock();
                }
                break;
			case MEDIA_TYPE_SEQUENCE:
				qDebug() << "nest";
                compose_sequence(c, render_audio);
                c->run_video_post_effect_stack();
                break;
            }
        }
    }
}

void ViewerWidget::paintGL() {
    bool loop = true;
    while (loop) {
        loop = false;
        texture_failed = false;

        if (multithreaded) retry_timer.stop();

        glClear(GL_COLOR_BUFFER_BIT);

        // compose video preview
        compose_sequence(NULL, (panel_timeline->playing || force_audio));

        // send audio to IO device
        if (panel_timeline->playing) {
            audio_sender_thread.cond.wakeAll();
        }

        if (texture_failed) {
            if (multithreaded) {
                retry_timer.start();
            } else {
                qDebug() << "[INFO] Texture failed - looping";
                loop = true;
            }
        }
    }
}

AudioSenderThread::AudioSenderThread() : close(false) {
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void AudioSenderThread::run() {
    lock.lock();
    while (true) {
        cond.wait(&lock);
        if (close) {
            break;
        } else {
            int written_bytes = 0;

            int adjusted_read_index = audio_ibuffer_read%audio_ibuffer_size;
            int max_write = audio_ibuffer_size - adjusted_read_index;
            int actual_write = send_audio_to_output(adjusted_read_index, max_write);
            written_bytes += actual_write;
            if (actual_write == max_write) {
                // got all the bytes, write again
                written_bytes += send_audio_to_output(0, audio_ibuffer_size);
            }
        }
    }
    lock.unlock();
}

int AudioSenderThread::send_audio_to_output(int offset, int max) {
    // send audio to device
    int actual_write = audio_io_device->write((const char*) audio_ibuffer+offset, max);
    audio_ibuffer_read += actual_write;

    // send samples to audio monitor cache
    if (panel_timeline->ui->audio_monitor->sample_cache_offset == -1) {
        panel_timeline->ui->audio_monitor->sample_cache_offset = panel_timeline->playhead;
    }
    long sample_cache_playhead = panel_timeline->ui->audio_monitor->sample_cache_offset + panel_timeline->ui->audio_monitor->sample_cache.size();
    int next_buffer_offset, buffer_offset_adjusted, i;
    int buffer_offset = get_buffer_offset_from_frame(sample_cache_playhead);
    samples.resize(av_get_channel_layout_nb_channels(sequence->audio_layout));
    samples.fill(0);
    while (buffer_offset < audio_ibuffer_read) {
        sample_cache_playhead++;
        next_buffer_offset = qMin(get_buffer_offset_from_frame(sample_cache_playhead), audio_ibuffer_read);
        while (buffer_offset < next_buffer_offset) {
            for (i=0;i<samples.size();i++) {
                buffer_offset_adjusted = buffer_offset%audio_ibuffer_size;
                samples[i] = qMax(qAbs((qint16) (((audio_ibuffer[buffer_offset_adjusted+1] & 0xFF) << 8) | (audio_ibuffer[buffer_offset_adjusted] & 0xFF))), samples[i]);
                buffer_offset += 2;
            }
        }
        panel_timeline->ui->audio_monitor->sample_cache.append(samples);
        buffer_offset = next_buffer_offset;
    }

    memset(audio_ibuffer+offset, 0, actual_write);

    return actual_write;
}
