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
#include <QOpenGLFramebufferObject>

extern "C" {
	#include <libavformat/avformat.h>
}

ViewerWidget::ViewerWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    multithreaded(true),
    force_audio(false),
    enable_paint(true),
	flip(false),
	fbo(NULL)
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
	/*if (fbo != NULL) {
		delete fbo;
		fbo = NULL;
	}*/

    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(deleteFunction()));

	initializeOpenGLFunctions();

    update();
}

//void ViewerWidget::resizeGL(int w, int h)
//{
//}

void ViewerWidget::paintEvent(QPaintEvent *e) {
    if (enable_paint) QOpenGLWidget::paintEvent(e);
}

GLuint ViewerWidget::draw_clip(GLuint texture) {
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	fbo->bind();
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); // top left
	glVertex2f(0, 0); // top left
	glTexCoord2f(1, 0); // top right
	glVertex2f(1, 0); // top right
	glTexCoord2f(1, 1); // bottom right
	glVertex2f(1, 1); // bottom right
	glTexCoord2f(0, 1); // bottom left
	glVertex2f(0, 1); // bottom left
	glEnd();
	glBindTexture(GL_TEXTURE_2D, NULL);
	fbo->release();
	glPopMatrix();
	return fbo->takeTexture();
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
                    if (m->get_stream_from_file_index(c->track < 0, c->media_stream) != NULL
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
						MediaStream* ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->track < 0, c->media_stream);

						int half_width = sequence->width/2;
						int half_height = sequence->height/2;
						if (flip) half_height = -half_height;
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						glColor4f(1.0, 1.0, 1.0, 1.0);
						glLoadIdentity();
						glOrtho(-half_width, half_width, half_height, -half_height, -1, 1);
						glViewport(0, 0, ms->video_width, ms->video_height);

						// TEMP DEBUG CODE
						if (fbo != NULL) delete fbo;
						fbo = new QOpenGLFramebufferObject(ms->video_width, ms->video_height);
						GLuint composite_texture = draw_clip(c->texture->textureId());
						// END

						GLTextureCoords coords;
						coords.vertexTopLeftX = coords.vertexBottomLeftX = -ms->video_width/2;
						coords.vertexTopLeftY = coords.vertexTopRightY = -ms->video_height/2;
						coords.vertexTopRightX = coords.vertexBottomRightX = ms->video_width/2;
						coords.vertexBottomLeftY = coords.vertexBottomRightY = ms->video_height/2;
						coords.textureTopLeftY = coords.textureTopRightY = coords.textureTopLeftX = coords.textureBottomLeftX = 0;
						coords.textureBottomLeftY = coords.textureBottomRightY = coords.textureTopRightX = coords.textureBottomRightX = 1.0;

						// EFFECT CODE START
                        for (int j=0;j<c->effects.size();j++) {
							if (c->effects.at(j)->enable_opengl && c->effects.at(j)->is_enabled()) {
								c->effects.at(j)->process_gl(((double)(panel_timeline->playhead-c->timeline_in+c->clip_in)/(double)sequence->frame_rate), coords);
								composite_texture = draw_clip(composite_texture);
							}
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
						// EFFECT CODE END

						for (int j=0;j<c->effects.size();j++) {
							if (c->effects.at(j)->enable_opengl && c->effects.at(j)->is_enabled()) {
								c->effects.at(j)->clean_gl();
							}
						}

						glViewport(0, 0, width(), height());

						glBindTexture(GL_TEXTURE_2D, composite_texture);

						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

						glBegin(GL_QUADS);
						glTexCoord2f(coords.textureTopLeftX, coords.textureTopLeftY); // top left
						glVertex2f(coords.vertexTopLeftX, coords.vertexTopLeftY); // top left
						glTexCoord2f(coords.textureTopRightX, coords.textureTopRightY); // top right
						glVertex2f(coords.vertexTopRightX, coords.vertexTopRightY); // top right
						glTexCoord2f(coords.textureBottomRightX, coords.textureBottomRightY); // bottom right
						glVertex2f(coords.vertexBottomRightX, coords.vertexBottomRightY); // bottom right
						glTexCoord2f(coords.textureBottomLeftX, coords.textureBottomLeftY); // bottom left
						glVertex2f(coords.vertexBottomLeftX, coords.vertexBottomLeftY); // bottom left
						glEnd();

						glDeleteTextures(1, &composite_texture);
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
	glClearColor(0, 0, 0, 1);
	glMatrixMode(GL_PROJECTION);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

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

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}
