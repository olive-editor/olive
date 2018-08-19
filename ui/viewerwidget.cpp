#include "viewerwidget.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "effects/effect.h"
#include "effects/transition.h"
#include "playback/playback.h"
#include "playback/audio.h"
#include "io/media.h"
#include "ui_timeline.h"

#include <QDebug>
#include <QPainter>
#include <QAudioOutput>
#include <QOpenGLShaderProgram>
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

    // error handler - retries after 250ms if we couldn't get the entire image
    retry_timer.setInterval(250);
	connect(&retry_timer, SIGNAL(timeout()), this, SLOT(retry()));
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

void ViewerWidget::compose_sequence(QVector<Clip*>& nests, bool render_audio) {
    Sequence* s = sequence;
	long playhead = panel_timeline->playhead;

	Clip* nest = NULL;
	if (nests.size() > 0) {
		nest = nests.last();
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

						QOpenGLShaderProgram shader;

						for (int j=0;j<c->effects.size();j++) {
							if (c->effects.at(j)->enable_opengl && c->effects.at(j)->is_enabled()) c->effects.at(j)->process_gl(panel_timeline->playhead-c->timeline_in+c->clip_in, shader, &anchor_x, &anchor_y);
						}

						if (c->opening_transition != NULL) {
							int transition_progress = playhead - c->timeline_in;
							if (transition_progress < c->opening_transition->length) {
								c->opening_transition->process_transition((double)transition_progress/(double)c->opening_transition->length);
							}
						}

						if (c->closing_transition != NULL) {
							int transition_progress = c->closing_transition->length - (playhead - c->timeline_in - c->getLength() + c->closing_transition->length);
							if (transition_progress < c->closing_transition->length) {
								c->closing_transition->process_transition((double)transition_progress/(double)c->closing_transition->length);
							}
						}

                        int anchor_right = ms->video_width - anchor_x;
                        int anchor_bottom = ms->video_height - anchor_y;

						bool use_gl_shaders = shader.link();
						if (use_gl_shaders) shader.bind();

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

						if (use_gl_shaders) shader.release();
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
				nests.append(c);
				compose_sequence(nests, render_audio);
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
		QVector<Clip*> nests;
		compose_sequence(nests, (panel_timeline->playing || force_audio));

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
