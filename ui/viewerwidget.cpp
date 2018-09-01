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
#include "playback/cacher.h"

#include <QDebug>
#include <QPainter>
#include <QAudioOutput>
#include <QOpenGLShaderProgram>
#include <QtMath>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

extern "C" {
	#include <libavformat/avformat.h>
}

ViewerWidget::ViewerWidget(QWidget *parent) :
    QOpenGLWidget(parent),
	rendering(false),
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
	makeCurrent();
	closeActiveClips(sequence, true);
	doneCurrent();
}

void ViewerWidget::retry() {
	update();
}

void ViewerWidget::initializeGL() {
	connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(deleteFunction()), Qt::DirectConnection);

	retry_timer.start();
}

//void ViewerWidget::resizeGL(int w, int h)
//{
//}

void ViewerWidget::paintEvent(QPaintEvent *e) {
	if (!rendering) {
		makeCurrent();
		QOpenGLWidget::paintEvent(e);
	}
}

GLuint ViewerWidget::draw_clip(Clip* clip, GLuint texture) {
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	clip->fbo->bind();

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

	clip->fbo->release();
	if (fbo != NULL) fbo->bind();

	glPopMatrix();
	return clip->fbo->takeTexture();
}

GLuint ViewerWidget::compose_sequence(Clip* nest, bool render_audio) {
	Sequence* s = sequence;
	long playhead = panel_timeline->playhead;

	if (nest != NULL) {
		s = static_cast<Sequence*>(nest->media);
		playhead += nest->clip_in - nest->timeline_in;
		playhead = refactor_frame_number(playhead, nest->sequence->frame_rate, s->frame_rate);
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
							open_clip(c, !rendering);
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
				if (is_clip_active(c, playhead)) {
					if (!c->open) open_clip(c, !rendering);
					clip_is_active = true;
				} else if (c->open) {
					close_clip(c);
				}
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

	int half_width = s->width/2;
	int half_height = s->height/2;
	if (rendering || nest != NULL) half_height = -half_height;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glLoadIdentity();
	glOrtho(-half_width, half_width, half_height, -half_height, -1, 1);

    for (int i=0;i<current_clips.size();i++) {
        Clip* c = current_clips.at(i);

        if (c->media_type == MEDIA_TYPE_FOOTAGE && !c->finished_opening) {
            qDebug() << "[WARNING] Tried to display clip" << i << "but it's closed";
            texture_failed = true;
        } else {
			if (c->track < 0) {
				GLuint textureID = 0;
				int video_width, video_height;

				if (c->media_type == MEDIA_TYPE_FOOTAGE) {
					get_clip_frame(c, playhead);
					MediaStream* ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->track < 0, c->media_stream);
					video_width = ms->video_width;
					video_height = ms->video_height;
					if (c->texture != NULL) textureID = c->texture->textureId();
				} else if (c->media_type == MEDIA_TYPE_SEQUENCE) {
                    Sequence* cs = static_cast<Sequence*>(c->media);
                    video_width = cs->width;
                    video_height = cs->height;
					textureID = compose_sequence(c, render_audio);
				}

				if (textureID == 0) {
					qDebug() << "[WARNING] Texture hasn't been created yet";
					texture_failed = true;
				} else if (playhead >= c->timeline_in) {
					// start preparing cache
					if (c->fbo == NULL) {
						c->fbo = new QOpenGLFramebufferObject(video_width, video_height);
					}

					glViewport(0, 0, video_width, video_height);

					GLuint composite_texture = draw_clip(c, textureID);

					GLTextureCoords coords;
					coords.vertexTopLeftX = coords.vertexBottomLeftX = -video_width/2;
					coords.vertexTopLeftY = coords.vertexTopRightY = -video_height/2;
					coords.vertexTopRightX = coords.vertexBottomRightX = video_width/2;
					coords.vertexBottomLeftY = coords.vertexBottomRightY = video_height/2;
					coords.textureTopLeftY = coords.textureTopRightY = coords.textureTopLeftX = coords.textureBottomLeftX = 0;
					coords.textureBottomLeftY = coords.textureBottomRightY = coords.textureTopRightX = coords.textureBottomRightX = 1.0;

					// EFFECT CODE START
					for (int j=0;j<c->effects.size();j++) {
						if (c->effects.at(j)->enable_opengl && c->effects.at(j)->is_enabled()) {
							for (int k=0;k<c->effects.at(j)->getIterations();k++) {
								c->effects.at(j)->process_gl(((double)(panel_timeline->playhead-c->timeline_in+c->clip_in)/(double)sequence->frame_rate), coords);
								composite_texture = draw_clip(c, composite_texture);
							}
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

					if (nest != NULL) {
						if (nest->fbo == NULL) nest->fbo = new QOpenGLFramebufferObject(s->width, s->height);
						nest->fbo->bind();
						glViewport(0, 0, s->width, s->height);
					} else if (rendering) {
						glViewport(0, 0, s->width, s->height);
					} else {
						glViewport(0, 0, width(), height());
					}

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

					if (nest != NULL) {
						nest->fbo->release();
						if (fbo != NULL) fbo->bind();
					}
				}
			} else {
				if (c->media_type == MEDIA_TYPE_FOOTAGE
						&& render_audio
						&& c->lock.tryLock()) {
					// clip is not caching, start caching audio
					cache_clip(c, playhead, false, false, c->audio_reset, nest);
					c->lock.unlock();
				} else if (c->media_type == MEDIA_TYPE_SEQUENCE) {
					compose_sequence(c, render_audio);
				}
			}
        }
    }

	return (nest != NULL && nest->fbo != NULL) ? nest->fbo->takeTexture() : 0;
}

void ViewerWidget::paintGL() {
	bool loop = false;
	do {
		loop = false;

		glClearColor(0, 0, 0, 1);
		glMatrixMode(GL_PROJECTION);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

        texture_failed = false;

		if (!rendering) retry_timer.stop();

        glClear(GL_COLOR_BUFFER_BIT);

		// compose video preview
		compose_sequence(NULL, (panel_timeline->playing || rendering));

        if (texture_failed) {
			if (rendering) {
				qDebug() << "[INFO] Texture failed - looping";
				loop = true;
            } else {
				retry_timer.start();
            }
        }

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	} while (loop);
}
