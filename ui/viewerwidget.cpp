#include "viewerwidget.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "project/effect.h"
#include "playback/playback.h"
#include "playback/audio.h"
#include "io/media.h"
#include "ui_timeline.h"
#include "playback/cacher.h"
#include "io/config.h"
#include "debug.h"
#include "io/math.h"

#include <QPainter>
#include <QAudioOutput>
#include <QOpenGLShaderProgram>
#include <QtMath>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QMenu>
#include <QOffscreenSurface>
#include <QFileDialog>

extern "C" {
	#include <libavformat/avformat.h>
}

ViewerWidget::ViewerWidget(QWidget *parent) :
	QOpenGLWidget(parent),
	default_fbo(NULL),
	waveform(false),
	dragging(false)
{
	setFocusPolicy(Qt::ClickFocus);

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	setFormat(format);

	// error handler - retries after 500ms if we couldn't get the entire image
	retry_timer.setInterval(500);
	connect(&retry_timer, SIGNAL(timeout()), this, SLOT(retry()));

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));
}

void ViewerWidget::deleteFunction() {
    // destroy all textures as well
	makeCurrent();
	closeActiveClips(viewer->seq, true);
	doneCurrent();
}

void ViewerWidget::show_context_menu() {
	QMenu menu(this);

	QAction* save_frame_as_image = menu.addAction("Save Frame as Image...");
	connect(save_frame_as_image, SIGNAL(triggered(bool)), this, SLOT(save_frame()));

	menu.exec(QCursor::pos());
}

void ViewerWidget::save_frame() {
	QString fn = QFileDialog::getSaveFileName(this, "Save Frame", QString(), "Portable Network Graphic (*.png);;JPEG (*.jpg *.jpeg);;Windows Bitmap (*.bmp);;Portable Pixmap (*.ppm);;X11 Bitmap (*.xbm);;X11 Pixmap (*.xpm)");

	if (!fn.isEmpty()) {
		QOpenGLFramebufferObject fbo(viewer->seq->width, viewer->seq->height, QOpenGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_RECTANGLE);

		rendering = true;
		fbo.bind();

		panel_sequence_viewer->viewer_widget->default_fbo = &fbo;

		paintGL();

		QImage img(viewer->seq->width, viewer->seq->height, QImage::Format_RGBA8888);
		glReadPixels(0, 0, img.width(), img.height(), GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
		img.save(fn);

		fbo.release();
		panel_sequence_viewer->viewer_widget->default_fbo = NULL;
		rendering = false;
	}
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

void ViewerWidget::seek_from_click(int x) {
	viewer->seek(getFrameFromScreenPoint((double) width() / (double) waveform_clip->timeline_out, x));
}

void ViewerWidget::mousePressEvent(QMouseEvent* event) {
	if (waveform) seek_from_click(event->x());
	dragging = true;
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event) {
	if (dragging) {
		if (waveform) {
			seek_from_click(event->x());
		} else {
			QDrag* drag = new QDrag(this);
			QMimeData* mimeData = new QMimeData;
			mimeData->setText("h");
			drag->setMimeData(mimeData);
			drag->exec();
		}
	}
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent *) {
	dragging = false;
}

void ViewerWidget::drawTitleSafeArea() {
    double halfWidth = 0.5;
    double halfHeight = 0.5;
    double viewportAr = (double) width() / (double) height();
    double halfAr = viewportAr*0.5;

    if (config.use_custom_title_safe_ratio && config.custom_title_safe_ratio > 0) {
        if (config.custom_title_safe_ratio > viewportAr) {
            halfHeight = (config.custom_title_safe_ratio/viewportAr)*0.5;
        } else {
            halfWidth = (viewportAr/config.custom_title_safe_ratio)*0.5;
        }
    }

    glLoadIdentity();
    glOrtho(-halfWidth, halfWidth, halfHeight, -halfHeight, -1, 1);

	glColor4f(0.66f, 0.66f, 0.66f, 1.0f);
    glBegin(GL_LINES);

    // action safe rectangle
    glVertex2d(-0.45, -0.45);
    glVertex2d(0.45, -0.45);
    glVertex2d(0.45, -0.45);
    glVertex2d(0.45, 0.45);
    glVertex2d(0.45, 0.45);
    glVertex2d(-0.45, 0.45);
    glVertex2d(-0.45, 0.45);
    glVertex2d(-0.45, -0.45);

    // title safe rectangle
    glVertex2d(-0.4, -0.4);
    glVertex2d(0.4, -0.4);
    glVertex2d(0.4, -0.4);
    glVertex2d(0.4, 0.4);
    glVertex2d(0.4, 0.4);
    glVertex2d(-0.4, 0.4);
    glVertex2d(-0.4, 0.4);
    glVertex2d(-0.4, -0.4);

    // horizontal centers
    glVertex2d(-0.45, 0);
    glVertex2d(-0.375, 0);
    glVertex2d(0.45, 0);
    glVertex2d(0.375, 0);

    // vertical centers
    glVertex2d(0, -0.45);
    glVertex2d(0, -0.375);
    glVertex2d(0, 0.45);
    glVertex2d(0, 0.375);

    glEnd();

    // center cross
    glLoadIdentity();
    glOrtho(-halfAr, halfAr, 0.5, -0.5, -1, 1);

    glBegin(GL_LINES);

    glVertex2d(-0.05, 0);
    glVertex2d(0.05, 0);
    glVertex2d(0, -0.05);
    glVertex2d(0, 0.05);

    glEnd();
}

GLuint ViewerWidget::draw_clip(QOpenGLFramebufferObject* fbo, GLuint texture) {
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	fbo->bind();

	glClear(GL_COLOR_BUFFER_BIT);

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
    glBindTexture(GL_TEXTURE_2D, 0);

	fbo->release();
	if (default_fbo != NULL) default_fbo->bind();

	glPopMatrix();
	return fbo->texture();
}

void ViewerWidget::process_effect(Clip* c, Effect* e, double timecode, GLTextureCoords& coords, GLuint& composite_texture, bool& fbo_switcher) {
	if (e->is_enabled()) {
		if (e->enable_coords) {
			e->process_coords(timecode, coords);
		}
		if (e->enable_shader || e->enable_superimpose) {
			e->startEffect();
			if (e->enable_shader) {
				e->process_shader(timecode, coords);
			}
			composite_texture = draw_clip(c->fbo[fbo_switcher], composite_texture);
			if (e->enable_superimpose) {
				GLuint superimpose_texture = e->process_superimpose(timecode);
				if (superimpose_texture != 0) draw_clip(c->fbo[fbo_switcher], superimpose_texture);
			}
			fbo_switcher = !fbo_switcher;
		}
	}
}

GLuint ViewerWidget::compose_sequence(QVector<Clip*>& nests, bool render_audio) {
	Sequence* s = viewer->seq;
	long playhead = s->playhead;

	if (!nests.isEmpty()) {
		for (int i=0;i<nests.size();i++) {
			s = static_cast<Sequence*>(nests.at(i)->media);
			playhead += nests.at(i)->clip_in - nests.at(i)->timeline_in;
			playhead = refactor_frame_number(playhead, nests.at(i)->sequence->frame_rate, s->frame_rate);
		}

		if (nests.last()->fbo != NULL) {
			nests.last()->fbo[0]->bind();
			glClear(GL_COLOR_BUFFER_BIT);
			nests.last()->fbo[0]->release();
		}
	}

	int audio_track_count = 0;

    QVector<Clip*> current_clips;

    for (int i=0;i<s->clips.size();i++) {
		Clip* c = s->clips.at(i);

        // if clip starts within one second and/or hasn't finished yet
		if (c != NULL) {
			if (!(!nests.isEmpty() && !same_sign(c->track, nests.last()->track))) {
				bool clip_is_active = false;

				switch (c->media_type) {
				case MEDIA_TYPE_FOOTAGE:
				{
					Media* m = static_cast<Media*>(c->media);
					if (m->ready) {
						MediaStream* ms = m->get_stream_from_file_index(c->track < 0, c->media_stream);
						if (ms != NULL && is_clip_active(c, playhead)) {
							// if thread is already working, we don't want to touch this,
							// but we also don't want to hang the UI thread
							if (!c->open) {
								open_clip(c, !rendering);
							}
							clip_is_active = true;
							if (c->track >= 0) audio_track_count++;
						} else if (c->open) {
							close_clip(c);
						}
					} else {
						texture_failed = true;
					}
				}
					break;
				case MEDIA_TYPE_SEQUENCE:
				case MEDIA_TYPE_SOLID:
				case MEDIA_TYPE_TONE:
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
	}

	int half_width = s->width/2;
	int half_height = s->height/2;
	if (rendering || !nests.isEmpty()) half_height = -half_height; // invert vertical
	glPushMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glLoadIdentity();
	glOrtho(-half_width, half_width, half_height, -half_height, -1, 1);

    for (int i=0;i<current_clips.size();i++) {
		Clip* c = current_clips.at(i);
        if (c->media_type == MEDIA_TYPE_FOOTAGE && !c->finished_opening) {
			dout << "[WARNING] Tried to display clip" << i << "but it's closed";
            texture_failed = true;
        } else {
			if (c->track < 0) {
				GLuint textureID = 0;
				int video_width = c->getWidth();
				int video_height = c->getHeight();

				if (c->media_type == MEDIA_TYPE_FOOTAGE) {
					// set up opengl texture
					if (c->texture == NULL) {
						c->texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
						c->texture->setSize(c->stream->codecpar->width, c->stream->codecpar->height);
						c->texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
						c->texture->setMipLevels(c->texture->maximumMipLevels());
						c->texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
						c->texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
					}
					get_clip_frame(c, playhead);
					textureID = c->texture->textureId();
				} else if (c->media_type == MEDIA_TYPE_SEQUENCE) {
					textureID = -1;
				}

				if (textureID == 0 && c->media_type != MEDIA_TYPE_SOLID) {
					dout << "[WARNING] Texture hasn't been created yet";
					texture_failed = true;
				} else if (playhead >= c->timeline_in) {
					glPushMatrix();

					// start preparing cache
					if (c->fbo == NULL) {
						c->fbo = new QOpenGLFramebufferObject* [2];
						c->fbo[0] = new QOpenGLFramebufferObject(video_width, video_height);
						c->fbo[1] = new QOpenGLFramebufferObject(video_width, video_height);
					}

					// clear fbos
					/*c->fbo[0]->bind();
					glClear(GL_COLOR_BUFFER_BIT);
					c->fbo[0]->release();
					c->fbo[1]->bind();
					glClear(GL_COLOR_BUFFER_BIT);
					c->fbo[1]->release();*/

					bool fbo_switcher = false;

					glViewport(0, 0, video_width, video_height);

					// for nested sequences
					if (c->media_type == MEDIA_TYPE_SEQUENCE) {
						nests.append(c);
						textureID = compose_sequence(nests, render_audio);
						nests.removeLast();
						fbo_switcher = true;
					}

					GLuint composite_texture;
					if (c->media_type == MEDIA_TYPE_SOLID) {
						composite_texture = c->fbo[fbo_switcher]->texture();
					} else {
						composite_texture = draw_clip(c->fbo[fbo_switcher], textureID);
					}

					fbo_switcher = !fbo_switcher;

                    GLTextureCoords coords;
                    coords.grid_size = 1;
					coords.vertexTopLeftX = coords.vertexBottomLeftX = -video_width/2;
					coords.vertexTopLeftY = coords.vertexTopRightY = -video_height/2;
					coords.vertexTopRightX = coords.vertexBottomRightX = video_width/2;
					coords.vertexBottomLeftY = coords.vertexBottomRightY = video_height/2;
					coords.textureTopLeftY = coords.textureTopRightY = coords.textureTopLeftX = coords.textureBottomLeftX = 0;
					coords.textureBottomLeftY = coords.textureBottomRightY = coords.textureTopRightX = coords.textureBottomRightX = 1.0;

					if (c->autoscale && (video_width != s->width || video_height != s->height)) {
						double width_multiplier = (double) s->width / (double) video_width;
						double height_multiplier = (double) s->height / (double) video_height;
						double scale_multiplier = qMin(width_multiplier, height_multiplier);
						glScalef(scale_multiplier, scale_multiplier, 1);
					}

					// EFFECT CODE START
					double timecode = ((double)(playhead-c->timeline_in+c->clip_in)/(double)c->sequence->frame_rate);
					for (int j=0;j<c->effects.size();j++) {
						process_effect(c, c->effects.at(j), timecode, coords, composite_texture, fbo_switcher);
					}

					if (c->opening_transition != NULL) {
						int transition_progress = playhead - c->timeline_in;
						if (transition_progress < c->opening_transition->length) {
							process_effect(c, c->opening_transition, (double)transition_progress/(double)c->opening_transition->length, coords, composite_texture, fbo_switcher);
							//c->opening_transition->process_transition((double)transition_progress/(double)c->opening_transition->length);
						}
					}

					if (c->closing_transition != NULL) {
						int transition_progress = c->closing_transition->length - (playhead - c->timeline_in - c->getLength() + c->closing_transition->length);
						if (transition_progress < c->closing_transition->length) {
							process_effect(c, c->closing_transition, (double)transition_progress/(double)c->closing_transition->length, coords, composite_texture, fbo_switcher);
							//c->closing_transition->process_transition((double)transition_progress/(double)c->closing_transition->length);
						}
					}

					for (int j=0;j<c->effects.size();j++) {
						if ((c->effects.at(j)->enable_shader || c->effects.at(j)->enable_superimpose) && c->effects.at(j)->is_enabled()) {
							c->effects.at(j)->endEffect();
						}
					}
					// EFFECT CODE END

					if (!nests.isEmpty()) {
						nests.last()->fbo[0]->bind();
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

                    // calculate the Q coordinate					
					/*double m1 = ((double)coords.vertexTopRightY - (double)coords.vertexBottomLeftY)/((double)coords.vertexTopRightX - (double)coords.vertexBottomLeftX);
                    double c1 = (double)coords.vertexBottomLeftY - m1 * (double)coords.vertexBottomLeftX;
                    double m2 = ((double)coords.vertexBottomRightY - (double)coords.vertexTopLeftY)/((double)coords.vertexBottomRightX - (double)coords.vertexTopLeftX);
                    double c2 = (double)coords.vertexTopLeftY - m2 * (double)coords.vertexTopLeftX;
                    double mid_x = (c2 - c1) / (m1 - m2);
					double mid_y = m1 * mid_x + c1;

                    double d0 = qSqrt(qPow(mid_x - (double)coords.vertexBottomLeftX, 2) + qPow(mid_y - (double)coords.vertexBottomLeftY, 2));
                    double d1 = qSqrt(qPow((double)coords.vertexBottomRightX - mid_x, 2) + qPow(mid_y - (double)coords.vertexBottomRightY, 2));
                    double d2 = qSqrt(qPow((double)coords.vertexTopRightX - mid_x, 2) + qPow((double)coords.vertexTopRightY - mid_y, 2));
                    double d3 = qSqrt(qPow(mid_x - (double)coords.vertexTopLeftX, 2) + qPow((double)coords.vertexTopLeftY - mid_y, 2));

                    double q0 = (d0 + d2)/d2;
                    double q1 = (d1 + d3)/d3;
                    double q2 = (d2 + d0)/d0;
					double q3 = (d3 + d1)/d1;*/

					if (coords.grid_size <= 1) {
						glTexCoord2f(coords.textureTopLeftX, coords.textureTopLeftY); // top left
                        glVertex2f(coords.vertexTopLeftX, coords.vertexTopLeftY); // top left
						glTexCoord2f(coords.textureTopRightX, coords.textureTopRightY); // top right
                        glVertex2f(coords.vertexTopRightX, coords.vertexTopRightY); // top right
						glTexCoord2f(coords.textureBottomRightX, coords.textureBottomRightY); // bottom right
                        glVertex2f(coords.vertexBottomRightX, coords.vertexBottomRightY); // bottom right
						glTexCoord2f(coords.textureBottomLeftX, coords.textureBottomLeftY); // bottom left
						glVertex2f(coords.vertexBottomLeftX, coords.vertexBottomLeftY); // bottom left
					} else {
                        double rows = coords.grid_size;
                        double cols = coords.grid_size;

                        for (double i=0;i<rows;i++) {
                            double row_prog = i/rows;
                            double next_row_prog = (i+1)/rows;
                            for (double j=0;j<cols;j++) {
                                double col_prog = j/cols;
                                double next_col_prog = (j+1)/cols;

                                double vertexTLX = double_lerp(coords.vertexTopLeftX, coords.vertexBottomLeftX, row_prog);
                                double vertexTRX = double_lerp(coords.vertexTopRightX, coords.vertexBottomRightX, row_prog);
                                double vertexBLX = double_lerp(coords.vertexTopLeftX, coords.vertexBottomLeftX, next_row_prog);
                                double vertexBRX = double_lerp(coords.vertexTopRightX, coords.vertexBottomRightX, next_row_prog);

                                double vertexTLY = double_lerp(coords.vertexTopLeftY, coords.vertexTopRightY, col_prog);
                                double vertexTRY = double_lerp(coords.vertexTopLeftY, coords.vertexTopRightY, next_col_prog);
                                double vertexBLY = double_lerp(coords.vertexBottomLeftY, coords.vertexBottomRightY, col_prog);
                                double vertexBRY = double_lerp(coords.vertexBottomLeftY, coords.vertexBottomRightY, next_col_prog);

                                glTexCoord2f(double_lerp(coords.textureTopLeftX, coords.textureTopRightX, col_prog), double_lerp(coords.textureTopLeftY, coords.textureBottomLeftY, row_prog)); // top left
                                glVertex2f(double_lerp(vertexTLX, vertexTRX, col_prog), double_lerp(vertexTLY, vertexBLY, row_prog)); // top left
                                glTexCoord2f(double_lerp(coords.textureTopLeftX, coords.textureTopRightX, next_col_prog), double_lerp(coords.textureTopRightY, coords.textureBottomRightY, row_prog)); // top right
                                glVertex2f(double_lerp(vertexTLX, vertexTRX, next_col_prog), double_lerp(vertexTRY, vertexBRY, row_prog)); // top right
                                glTexCoord2f(double_lerp(coords.textureBottomLeftX, coords.textureBottomRightX, next_col_prog), double_lerp(coords.textureTopRightY, coords.textureBottomRightY, next_row_prog)); // bottom right
                                glVertex2f(double_lerp(vertexBLX, vertexBRX, next_col_prog), double_lerp(vertexTRY, vertexBRY, next_row_prog)); // bottom right
                                glTexCoord2f(double_lerp(coords.textureBottomLeftX, coords.textureBottomRightX, col_prog), double_lerp(coords.textureTopLeftY, coords.textureBottomLeftY, next_row_prog)); // bottom left
                                glVertex2f(double_lerp(vertexBLX, vertexBRX, col_prog), double_lerp(vertexTLY, vertexBLY, next_row_prog)); // bottom left
                            }
                        }
					}

					glEnd();

                    glBindTexture(GL_TEXTURE_2D, 0); // unbind texture

					if (!nests.isEmpty()) {
						nests.last()->fbo[0]->release();
						if (default_fbo != NULL) default_fbo->bind();
					}

					glPopMatrix();
				}
            } else {
                if (render_audio || (config.enable_audio_scrubbing && audio_scrub)) {
					switch (c->media_type) {
					case MEDIA_TYPE_FOOTAGE:
					case MEDIA_TYPE_TONE:
						if (c->lock.tryLock()) {
                            // clip is not caching, start caching audio
							cache_clip(c, playhead, c->audio_reset, !render_audio, nests);
							c->lock.unlock();
						}
						break;
					case MEDIA_TYPE_SEQUENCE:
						nests.append(c);
						compose_sequence(nests, render_audio);
						nests.removeLast();
						break;
					}
				}

				// visually update all the keyframe values
				if (c->sequence == viewer->seq) { // only if you can currently see them
					double ts = (playhead - c->timeline_in + c->clip_in)/s->frame_rate;
					for (int i=0;i<c->effects.size();i++) {
						Effect* e = c->effects.at(i);
						for (int j=0;j<e->row_count();j++) {
							EffectRow* r = e->row(j);
							for (int k=0;k<r->fieldCount();k++) {
								r->field(k)->validate_keyframe_data(ts);
							}
						}
					}
				}
			}
        }
	}

	if (audio_track_count == 0) {
		viewer->play_wake();
	}

	glPopMatrix();

	if (!nests.isEmpty() && nests.last()->fbo != NULL) {
		// returns nested clip's texture
		return nests.last()->fbo[0]->texture();
	}

	return 0;
}

void ViewerWidget::paintGL() {
    bool render_audio = (viewer->playing || rendering);
	bool loop = false;
	do {
		loop = false;

		glClearColor(0, 0, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		texture_failed = false;

		if (!rendering) retry_timer.stop();

		glClear(GL_COLOR_BUFFER_BIT);

		// compose video preview
		glClearColor(0, 0, 0, 0);

		QVector<Clip*> nests;

		compose_sequence(nests, render_audio);

		if (waveform) {
			double waveform_zoom = (double) waveform_ms->audio_preview.size() / (double) width();
			double timeline_zoom = (double) width() / (double) waveform_clip->timeline_out;

			QPainter p(this);
			if (viewer->seq->using_workarea) {
				int in_x = getScreenPointFromFrame(timeline_zoom, viewer->seq->workarea_in);
				int out_x = getScreenPointFromFrame(timeline_zoom, viewer->seq->workarea_out);

				p.fillRect(QRect(in_x, 0, out_x - in_x, height()), QColor(255, 255, 255, 64));
				p.setPen(Qt::white);
				p.drawLine(in_x, 0, in_x, height());
				p.drawLine(out_x, 0, out_x, height());
			}
			p.setPen(Qt::green);
			draw_waveform(waveform_clip, waveform_ms, waveform_clip->timeline_out, &p, rect(), 0, width(), waveform_zoom);
			p.setPen(Qt::red);
			int playhead_x = getScreenPointFromFrame(timeline_zoom, viewer->seq->playhead);
			p.drawLine(playhead_x, 0, playhead_x, height());
		}

		if (texture_failed) {
			if (rendering) {
				dout << "[INFO] Texture failed - looping";
				loop = true;
			} else {
				retry_timer.start();
			}
		}

		if (config.show_title_safe_area && !rendering) {
			drawTitleSafeArea();
		}

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
    } while (loop);
}
