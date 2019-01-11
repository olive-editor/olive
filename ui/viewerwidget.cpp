#include "viewerwidget.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "project/effect.h"
#include "project/transition.h"
#include "playback/playback.h"
#include "playback/audio.h"
#include "project/footage.h"
#include "playback/cacher.h"
#include "io/config.h"
#include "debug.h"
#include "io/math.h"
#include "ui/collapsiblewidget.h"
#include "project/undo.h"
#include "project/media.h"
#include "ui/viewercontainer.h"
#include "io/avtogl.h"
#include "ui/timelinewidget.h"

#include <QPainter>
#include <QAudioOutput>
#include <QOpenGLShaderProgram>
#include <QtMath>
#include <QOpenGLFramebufferObject>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QMenu>
#include <QOffscreenSurface>
#include <QFileDialog>
#include <QPolygon>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QApplication>

extern "C" {
	#include <libavformat/avformat.h>
}

#define GL_DEFAULT_BLEND glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

ViewerWidget::ViewerWidget(QWidget *parent) :
	QOpenGLWidget(parent),
	default_fbo(NULL),
	waveform(false),
	dragging(false),
	selected_gizmo(NULL),
	waveform_zoom(1.0),
	waveform_scroll(0)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	setFormat(format);

	// error handler - retries after 50ms if we couldn't get the entire image
	retry_timer.setInterval(50);
	connect(&retry_timer, SIGNAL(timeout()), this, SLOT(retry()));

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));
}

void ViewerWidget::delete_function() {
	// destroy all textures as well
	if (viewer->seq != NULL) {
		makeCurrent();
		closeActiveClips(viewer->seq);
		doneCurrent();
	}
}

void ViewerWidget::set_waveform_scroll(int s) {
	if (waveform) {
		waveform_scroll = s;
		update();
	}
}

void ViewerWidget::show_context_menu() {
	QMenu menu(this);

	QAction* save_frame_as_image = menu.addAction("Save Frame as Image...");
	connect(save_frame_as_image, SIGNAL(triggered(bool)), this, SLOT(save_frame()));

	QAction* show_fullscreen_action = menu.addAction("Show Fullscreen");
	connect(show_fullscreen_action, SIGNAL(triggered()), this, SLOT(show_fullscreen()));

	QMenu zoom_menu("Zoom");
	QAction* fit_zoom = zoom_menu.addAction("Fit");
	connect(fit_zoom, SIGNAL(triggered(bool)), this, SLOT(set_fit_zoom()));
	zoom_menu.addAction("10%")->setData(0.1);
	zoom_menu.addAction("25%")->setData(0.25);
	zoom_menu.addAction("50%")->setData(0.5);
	zoom_menu.addAction("75%")->setData(0.75);
	zoom_menu.addAction("100%")->setData(1.0);
	zoom_menu.addAction("150%")->setData(1.5);
	zoom_menu.addAction("200%")->setData(2.0);
	zoom_menu.addAction("400%")->setData(4.0);
	QAction* custom_zoom = zoom_menu.addAction("Custom");
	connect(custom_zoom, SIGNAL(triggered(bool)), this, SLOT(set_custom_zoom()));
	connect(&zoom_menu, SIGNAL(triggered(QAction*)), this, SLOT(set_menu_zoom(QAction*)));
	menu.addMenu(&zoom_menu);

	menu.exec(QCursor::pos());
}

void ViewerWidget::save_frame() {
	QFileDialog fd(this);
	fd.setAcceptMode(QFileDialog::AcceptSave);
	fd.setFileMode(QFileDialog::AnyFile);
	fd.setWindowTitle("Save Frame");
	fd.setNameFilter("Portable Network Graphic (*.png);;JPEG (*.jpg);;Windows Bitmap (*.bmp);;Portable Pixmap (*.ppm);;X11 Bitmap (*.xbm);;X11 Pixmap (*.xpm)");

	if (fd.exec()) {
		QString fn = fd.selectedFiles().at(0);
		QString selected_ext = fd.selectedNameFilter().mid(fd.selectedNameFilter().indexOf(QRegExp("\\*.[a-z][a-z][a-z]")) + 1, 4);
		if (!fn.endsWith(selected_ext,  Qt::CaseInsensitive)) {
			fn += selected_ext;
		}
		QOpenGLFramebufferObject fbo(viewer->seq->width, viewer->seq->height, QOpenGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_RECTANGLE);

		rendering = true;
		fbo.bind();

		default_fbo = &fbo;

		paintGL();

		QImage img(viewer->seq->width, viewer->seq->height, QImage::Format_RGBA8888);
		glReadPixels(0, 0, img.width(), img.height(), GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
		img.save(fn);

		fbo.release();
		default_fbo = NULL;
		rendering = false;
	}
}

void ViewerWidget::show_fullscreen() {
	showFullScreen();
}

void ViewerWidget::set_fit_zoom() {
	container->fit = true;
	container->adjust();
}

void ViewerWidget::set_custom_zoom() {
	bool ok;
	double d = QInputDialog::getDouble(this, "Viewer Zoom", "Set Custom Zoom Value:", container->zoom*100, 0, 2147483647, 2, &ok);
	if (ok) {
		container->fit = false;
		container->zoom = d*0.01;
		container->adjust();
	}
}

void ViewerWidget::set_menu_zoom(QAction* action) {
	const QVariant& data = action->data();
	if (!data.isNull()) {
		container->fit = false;
		container->zoom = data.toDouble();
		container->adjust();
	}
}

void ViewerWidget::retry() {
	update();
}

void ViewerWidget::initializeGL() {
	initializeOpenGLFunctions();

	connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(delete_function()), Qt::DirectConnection);

	retry_timer.start();
}

//void ViewerWidget::resizeGL(int w, int h)
//{
//}

void ViewerWidget::paintEvent(QPaintEvent *e) {
	if (!rendering && context()->thread() == this->thread()) {
		makeCurrent();
		QOpenGLWidget::paintEvent(e);
	}
}

void ViewerWidget::seek_from_click(int x) {
	viewer->seek(getFrameFromScreenPoint(waveform_zoom, x+waveform_scroll));
}

EffectGizmo* ViewerWidget::get_gizmo_from_mouse(int x, int y) {
	if (gizmos != NULL) {
		double multiplier = (double) viewer->seq->width / (double) width();
		QPoint mouse_pos(qRound(x*multiplier), qRound(y*multiplier));
		int dot_size = 2 * qRound(GIZMO_DOT_SIZE * multiplier);
		int target_size = 2 * qRound(GIZMO_TARGET_SIZE * multiplier);
		for (int i=0;i<gizmos->gizmo_count();i++) {
			EffectGizmo* g = gizmos->gizmo(i);

			switch (g->get_type()) {
			case GIZMO_TYPE_DOT:
				if (mouse_pos.x() > g->screen_pos[0].x() - dot_size
						&& mouse_pos.y() > g->screen_pos[0].y() - dot_size
						&& mouse_pos.x() < g->screen_pos[0].x() + dot_size
						&& mouse_pos.y() < g->screen_pos[0].y() + dot_size) {
					return g;
				}
				break;
			case GIZMO_TYPE_POLY:
				if (QPolygon(g->screen_pos).containsPoint(mouse_pos, Qt::OddEvenFill)) {
					return g;
				}
				break;
			case GIZMO_TYPE_TARGET:
				if (mouse_pos.x() > g->screen_pos[0].x() - target_size
						&& mouse_pos.y() > g->screen_pos[0].y() - target_size
						&& mouse_pos.x() < g->screen_pos[0].x() + target_size
						&& mouse_pos.y() < g->screen_pos[0].y() + target_size) {
					return g;
				}
				break;
			}

		}
	}
	return NULL;
}

void ViewerWidget::move_gizmos(QMouseEvent *event, bool done) {
	if (selected_gizmo != NULL) {
		double multiplier = (double) viewer->seq->width / (double) width();

		int x_movement = (event->pos().x() - drag_start_x)*multiplier;
		int y_movement = (event->pos().y() - drag_start_y)*multiplier;

		gizmos->gizmo_move(selected_gizmo, x_movement, y_movement, get_timecode(gizmos->parent_clip, gizmos->parent_clip->sequence->playhead), done);

		gizmo_x_mvmt += x_movement;
		gizmo_y_mvmt += y_movement;

		drag_start_x = event->pos().x();
		drag_start_y = event->pos().y();

		gizmos->field_changed();
	}
}

void ViewerWidget::mousePressEvent(QMouseEvent* event) {
	if (waveform) {
		seek_from_click(event->x());
	} else if (event->buttons() & Qt::MiddleButton || panel_timeline->tool == TIMELINE_TOOL_HAND) {
		container->dragScrollPress(event->pos());
	} else {
		drag_start_x = event->pos().x();
		drag_start_y = event->pos().y();

		gizmo_x_mvmt = 0;
		gizmo_y_mvmt = 0;

		selected_gizmo = get_gizmo_from_mouse(event->pos().x(), event->pos().y());

		if (selected_gizmo != NULL) {
			selected_gizmo->set_previous_value();
		}
	}
	dragging = true;
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event) {
	unsetCursor();
	if (panel_timeline->tool == TIMELINE_TOOL_HAND) {
		setCursor(Qt::OpenHandCursor);
	}
	if (dragging) {
		if (waveform) {
			seek_from_click(event->x());
		} else if (event->buttons() & Qt::MiddleButton || panel_timeline->tool == TIMELINE_TOOL_HAND) {
			container->dragScrollMove(event->pos());
		} else if (gizmos == NULL) {
			QDrag* drag = new QDrag(this);
			QMimeData* mimeData = new QMimeData;
			mimeData->setText("h"); // QMimeData will fail without some kind of data
			drag->setMimeData(mimeData);
			drag->exec();
			dragging = false;
		} else {
			move_gizmos(event, false);
		}
	} else {
		EffectGizmo* g = get_gizmo_from_mouse(event->pos().x(), event->pos().y());
		if (g != NULL) {
			if (g->get_cursor() > -1) {
				setCursor(static_cast<enum Qt::CursorShape>(g->get_cursor()));
			}
		}
	}
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent *event) {
	if (dragging) move_gizmos(event, true);
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
	glOrtho(-halfWidth, halfWidth, halfHeight, -halfHeight, 0, 1);

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

GLuint ViewerWidget::draw_clip(QOpenGLFramebufferObject* fbo, GLuint texture, bool clear) {
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	fbo->bind();

	if (clear) glClear(GL_COLOR_BUFFER_BIT);

	// get current blend mode
	GLint src_rgb, src_alpha, dst_rgb, dst_alpha;
	glGetIntegerv(GL_BLEND_SRC_RGB, &src_rgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &src_alpha);
	glGetIntegerv(GL_BLEND_DST_RGB, &dst_rgb);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &dst_alpha);

	GL_DEFAULT_BLEND

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

	// restore previous blendFunc
	glBlendFuncSeparate(src_rgb, dst_rgb, src_alpha, dst_alpha);

	if (default_fbo != NULL) default_fbo->bind();

	glPopMatrix();
	return fbo->texture();
}

void ViewerWidget::process_effect(Clip* c, Effect* e, double timecode, GLTextureCoords& coords, GLuint& composite_texture, bool& fbo_switcher, int data) {
	if (e->is_enabled()) {
		if (e->enable_coords) {
			e->process_coords(timecode, coords, data);
		}
		if (e->enable_shader || e->enable_superimpose) {
			e->startEffect();
			if (e->enable_shader && e->is_glsl_linked()) {
				e->process_shader(timecode, coords);
				composite_texture = draw_clip(c->fbo[fbo_switcher], composite_texture, true);
				fbo_switcher = !fbo_switcher;
			}
			if (e->enable_superimpose) {
				GLuint superimpose_texture = e->process_superimpose(timecode);
				if (superimpose_texture == 0) {
					dout << "[WARNING] Superimpose texture was NULL, retrying...";
					texture_failed = true;
				} else {
					composite_texture = draw_clip(c->fbo[!fbo_switcher], superimpose_texture, false);
				}
			}
			e->endEffect();
		}
	}
}

int motion_blur_prog = 0;
int motion_blur_lim = 4;

GLuint ViewerWidget::compose_sequence(QVector<Clip*>& nests, bool render_audio) {
	Sequence* s = viewer->seq;
	long playhead = s->playhead;

	if (!nests.isEmpty()) {
		for (int i=0;i<nests.size();i++) {
			s = nests.at(i)->media->to_sequence();
			playhead += nests.at(i)->clip_in - nests.at(i)->get_timeline_in_with_transition();
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

				if (c->media != NULL && c->media->get_type() == MEDIA_TYPE_FOOTAGE) {
					Footage* m = c->media->to_footage();
					if (!m->invalid && !(c->track >= 0 && !is_audio_device_set())) {
						if (m->ready) {
							const FootageStream* ms = m->get_stream_from_file_index(c->track < 0, c->media_stream);
							if (ms != NULL && is_clip_active(c, playhead)) {
								// if thread is already working, we don't want to touch this,
								// but we also don't want to hang the UI thread
								if (!c->open) {
									open_clip(c, !rendering);
								}
								clip_is_active = true;
								if (c->track >= 0) audio_track_count++;
							} else if (c->open) {
								close_clip(c, false);
							}
						} else {
							//dout << "[WARNING] Media '" + m->name + "' was not ready, retrying...";
							texture_failed = true;
						}
					}
				} else {
					if (is_clip_active(c, playhead)) {
						if (!c->open) open_clip(c, !rendering);
						clip_is_active = true;
					} else if (c->open) {
						close_clip(c, false);
					}
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
	glLoadIdentity();
	glOrtho(-half_width, half_width, half_height, -half_height, -1, 10);

	for (int i=0;i<current_clips.size();i++) {
		GL_DEFAULT_BLEND
		glColor4f(1.0, 1.0, 1.0, 1.0);

		Clip* c = current_clips.at(i);

		if (c->media != NULL && c->media->get_type() == MEDIA_TYPE_FOOTAGE && !c->finished_opening) {
			dout << "[WARNING] Tried to display clip" << i << "but it's closed";
			texture_failed = true;
		} else {
			if (c->track < 0) {
				GLuint textureID = 0;
				int video_width = c->getWidth();
				int video_height = c->getHeight();

				if (c->media != NULL) {
					switch (c->media->get_type()) {
					case MEDIA_TYPE_FOOTAGE:
						// set up opengl texture
						if (c->texture == NULL) {
							c->texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
							c->texture->setSize(c->stream->codecpar->width, c->stream->codecpar->height);
							c->texture->setFormat(get_gl_tex_fmt_from_av(c->pix_fmt));
							c->texture->setMipLevels(c->texture->maximumMipLevels());
							c->texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
							c->texture->allocateStorage(get_gl_pix_fmt_from_av(c->pix_fmt), QOpenGLTexture::UInt8);
						}
						get_clip_frame(c, playhead);
						textureID = c->texture->textureId();
						break;
					case MEDIA_TYPE_SEQUENCE:
						textureID = -1;
						break;
					}
				}

				if (textureID == 0 && c->media != NULL) {
					dout << "[WARNING] Texture hasn't been created yet";
					texture_failed = true;
				} else if (playhead >= c->get_timeline_in_with_transition()) {
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

					GLuint composite_texture;

					if (c->media == NULL) {
						c->fbo[fbo_switcher]->bind();
						glClear(GL_COLOR_BUFFER_BIT);
						c->fbo[fbo_switcher]->release();
						composite_texture = c->fbo[fbo_switcher]->texture();
					} else {
						// for nested sequences
						if (c->media->get_type()== MEDIA_TYPE_SEQUENCE) {
							nests.append(c);
							textureID = compose_sequence(nests, render_audio);
							nests.removeLast();
							fbo_switcher = true;
						}

						composite_texture = draw_clip(c->fbo[fbo_switcher], textureID, true);
					}

					fbo_switcher = !fbo_switcher;

					// set up default coords
					GLTextureCoords coords;
					coords.grid_size = 1;
					coords.vertexTopLeftX = coords.vertexBottomLeftX = -video_width/2;
					coords.vertexTopLeftY = coords.vertexTopRightY = -video_height/2;
					coords.vertexTopRightX = coords.vertexBottomRightX = video_width/2;
					coords.vertexBottomLeftY = coords.vertexBottomRightY = video_height/2;
					coords.vertexBottomLeftZ = coords.vertexBottomRightZ = coords.vertexTopLeftZ = coords.vertexTopRightZ = 1;
					coords.textureTopLeftY = coords.textureTopRightY = coords.textureTopLeftX = coords.textureBottomLeftX = 0.0;
					coords.textureBottomLeftY = coords.textureBottomRightY = coords.textureTopRightX = coords.textureBottomRightX = 1.0;
					coords.textureTopLeftQ = coords.textureTopRightQ = coords.textureTopLeftQ = coords.textureBottomLeftQ = 1;

					// set up autoscale
					if (c->autoscale && (video_width != s->width && video_height != s->height)) {
						float width_multiplier = (float) s->width / (float) video_width;
						float height_multiplier = (float) s->height / (float) video_height;
						float scale_multiplier = qMin(width_multiplier, height_multiplier);
						glScalef(scale_multiplier, scale_multiplier, 1);
					}

					// EFFECT CODE START
					double timecode = get_timecode(c, playhead);

					Effect* first_gizmo_effect = NULL;
					Effect* selected_effect = NULL;

					for (int j=0;j<c->effects.size();j++) {
						Effect* e = c->effects.at(j);
						process_effect(c, e, timecode, coords, composite_texture, fbo_switcher, TA_NO_TRANSITION);

						if (e->are_gizmos_enabled()) {
							if (first_gizmo_effect == NULL) first_gizmo_effect = e;
							if (e->container->selected) selected_effect = e;
						}
					}

					if (!rendering) {
						if (selected_effect != NULL) {
							gizmos = selected_effect;
						} else if (panel_timeline->is_clip_selected(c, true)) {
							gizmos = first_gizmo_effect;
						}
					}

					if (c->get_opening_transition() != NULL) {
						int transition_progress = playhead - c->get_timeline_in_with_transition();
						if (transition_progress < c->get_opening_transition()->get_length()) {
							process_effect(c, c->get_opening_transition(), (double)transition_progress/(double)c->get_opening_transition()->get_length(), coords, composite_texture, fbo_switcher, TA_OPENING_TRANSITION);
						}
					}

					if (c->get_closing_transition() != NULL) {
						int transition_progress = playhead - (c->get_timeline_out_with_transition() - c->get_closing_transition()->get_length());
						if (transition_progress >= 0 && transition_progress < c->get_closing_transition()->get_length()) {
							process_effect(c, c->get_closing_transition(), (double)transition_progress/(double)c->get_closing_transition()->get_length(), coords, composite_texture, fbo_switcher, TA_CLOSING_TRANSITION);
						}
					}
					// EFFECT CODE END

					if (!nests.isEmpty()) {
						nests.last()->fbo[0]->bind();
						glViewport(0, 0, s->width, s->height);
					} else if (rendering) {
						glViewport(0, 0, s->width, s->height);
					} else {
						int widget_width = width();
						int widget_height = height();

						widget_width *= QApplication::desktop()->devicePixelRatio();
						widget_height *= QApplication::desktop()->devicePixelRatio();

						glViewport(0, 0, widget_width, widget_height);
					}

					glBindTexture(GL_TEXTURE_2D, composite_texture);

					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					glBegin(GL_QUADS);

					if (coords.grid_size <= 1) {
						float z = 0.0f;

						glTexCoord2f(coords.textureTopLeftX, coords.textureTopLeftY); // top left
						glVertex3f(coords.vertexTopLeftX, coords.vertexTopLeftY, z); // top left
						glTexCoord2f(coords.textureTopRightX, coords.textureTopRightY); // top right
						glVertex3f(coords.vertexTopRightX, coords.vertexTopRightY, z); // top right
						glTexCoord2f(coords.textureBottomRightX, coords.textureBottomRightY); // bottom right
						glVertex3f(coords.vertexBottomRightX, coords.vertexBottomRightY, z); // bottom right
						glTexCoord2f(coords.textureBottomLeftX, coords.textureBottomLeftY); // bottom left
						glVertex3f(coords.vertexBottomLeftX, coords.vertexBottomLeftY, z); // bottom left
					} else {
						float rows = coords.grid_size;
						float cols = coords.grid_size;

						for (float k=0;k<rows;k++) {
							float row_prog = k/rows;
							float next_row_prog = (k+1)/rows;
							for (float j=0;j<cols;j++) {
								float col_prog = j/cols;
								float next_col_prog = (j+1)/cols;

								float vertexTLX = float_lerp(coords.vertexTopLeftX, coords.vertexBottomLeftX, row_prog);
								float vertexTRX = float_lerp(coords.vertexTopRightX, coords.vertexBottomRightX, row_prog);
								float vertexBLX = float_lerp(coords.vertexTopLeftX, coords.vertexBottomLeftX, next_row_prog);
								float vertexBRX = float_lerp(coords.vertexTopRightX, coords.vertexBottomRightX, next_row_prog);

								float vertexTLY = float_lerp(coords.vertexTopLeftY, coords.vertexTopRightY, col_prog);
								float vertexTRY = float_lerp(coords.vertexTopLeftY, coords.vertexTopRightY, next_col_prog);
								float vertexBLY = float_lerp(coords.vertexBottomLeftY, coords.vertexBottomRightY, col_prog);
								float vertexBRY = float_lerp(coords.vertexBottomLeftY, coords.vertexBottomRightY, next_col_prog);

								glTexCoord2f(float_lerp(coords.textureTopLeftX, coords.textureTopRightX, col_prog), float_lerp(coords.textureTopLeftY, coords.textureBottomLeftY, row_prog)); // top left
								glVertex2f(float_lerp(vertexTLX, vertexTRX, col_prog), float_lerp(vertexTLY, vertexBLY, row_prog)); // top left
								glTexCoord2f(float_lerp(coords.textureTopLeftX, coords.textureTopRightX, next_col_prog), float_lerp(coords.textureTopRightY, coords.textureBottomRightY, row_prog)); // top right
								glVertex2f(float_lerp(vertexTLX, vertexTRX, next_col_prog), float_lerp(vertexTRY, vertexBRY, row_prog)); // top right
								glTexCoord2f(float_lerp(coords.textureBottomLeftX, coords.textureBottomRightX, next_col_prog), float_lerp(coords.textureTopRightY, coords.textureBottomRightY, next_row_prog)); // bottom right
								glVertex2f(float_lerp(vertexBLX, vertexBRX, next_col_prog), float_lerp(vertexTRY, vertexBRY, next_row_prog)); // bottom right
								glTexCoord2f(float_lerp(coords.textureBottomLeftX, coords.textureBottomRightX, col_prog), float_lerp(coords.textureTopLeftY, coords.textureBottomLeftY, next_row_prog)); // bottom left
								glVertex2f(float_lerp(vertexBLX, vertexBRX, col_prog), float_lerp(vertexTLY, vertexBLY, next_row_prog)); // bottom left
							}
						}
					}

					glEnd();

					glBindTexture(GL_TEXTURE_2D, 0); // unbind texture

					if (gizmos != NULL && !drawn_gizmos) {
						gizmos->gizmo_draw(timecode, coords); // set correct gizmo coords
						gizmos->gizmo_world_to_screen();

						drawn_gizmos = true;
					}

					if (!nests.isEmpty()) {
						nests.last()->fbo[0]->release();
						if (default_fbo != NULL) default_fbo->bind();
					}

					glPopMatrix();

					/*GLfloat motion_blur_frac = (GLfloat) motion_blur_prog / (GLfloat) motion_blur_lim;
					if (motion_blur_prog == 0) {
						glAccum(GL_LOAD, motion_blur_frac);
					} else {
						glAccum(GL_ACCUM, motion_blur_frac);
					}
					motion_blur_prog++;*/
				}
			} else {
				if (render_audio || (config.enable_audio_scrubbing && audio_scrub)) {
					if (c->media != NULL && c->media->get_type() == MEDIA_TYPE_SEQUENCE) {
						nests.append(c);
						compose_sequence(nests, render_audio);
						nests.removeLast();
					} else {
						if (c->lock.tryLock()) {
							// clip is not caching, start caching audio
							cache_clip(c, playhead, c->audio_reset, !render_audio, nests);
							c->lock.unlock();
						}
					}
				}

				// visually update all the keyframe values
				if (c->sequence == viewer->seq) { // only if you can currently see them
					double ts = (playhead - c->get_timeline_in_with_transition() + c->get_clip_in_with_transition())/s->frame_rate;
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
	drawn_gizmos = false;
	force_quit = false;
	if (viewer->seq != NULL) {
		gizmos = NULL;

		bool render_audio = (viewer->playing || rendering);
		bool loop = false;
		do {
			loop = false;

			glClearColor(0, 0, 0, 1);
			glMatrixMode(GL_MODELVIEW);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glEnable(GL_DEPTH);

			texture_failed = false;

			if (!rendering) retry_timer.stop();

			glClear(GL_COLOR_BUFFER_BIT);

			// compose video preview
			glClearColor(0, 0, 0, 0);

			QVector<Clip*> nests;

			compose_sequence(nests, render_audio);

			if (waveform) {
				QPainter p(this);
				if (viewer->seq->using_workarea) {
					int in_x = getScreenPointFromFrame(waveform_zoom, viewer->seq->workarea_in) - waveform_scroll;
					int out_x = getScreenPointFromFrame(waveform_zoom, viewer->seq->workarea_out) - waveform_scroll;

					p.fillRect(QRect(in_x, 0, out_x - in_x, height()), QColor(255, 255, 255, 64));
					p.setPen(Qt::white);
					p.drawLine(in_x, 0, in_x, height());
					p.drawLine(out_x, 0, out_x, height());
				}
				QRect wr = rect();
				wr.setX(wr.x() - waveform_scroll);

				p.setPen(Qt::green);
				draw_waveform(waveform_clip, waveform_ms, waveform_clip->timeline_out, &p, wr, waveform_scroll, width()+waveform_scroll, waveform_zoom);
				p.setPen(Qt::red);
				int playhead_x = getScreenPointFromFrame(waveform_zoom, viewer->seq->playhead) - waveform_scroll;
				p.drawLine(playhead_x, 0, playhead_x, height());
			}

			if (force_quit) break;
			if (texture_failed) {
				if (rendering) {
					dout << "[INFO] Texture failed - looping";
					loop = true;
				} else if (!viewer->playing) {
					retry_timer.start();
				}
			}

			if (config.show_title_safe_area && !rendering) {
				drawTitleSafeArea();
			}

			if (gizmos != NULL && drawn_gizmos) {
				float color[4];
				glGetFloatv(GL_CURRENT_COLOR, color);

				float dot_size = GIZMO_DOT_SIZE / width() * viewer->seq->width;
				float target_size = GIZMO_TARGET_SIZE / width() * viewer->seq->width;

				glPushMatrix();
				glLoadIdentity();
				glOrtho(0, viewer->seq->width, viewer->seq->height, 0, -1, 10);
				float gizmo_z = 0.0f;
				for (int j=0;j<gizmos->gizmo_count();j++) {
					EffectGizmo* g = gizmos->gizmo(j);
					glColor4f(g->color.redF(), g->color.greenF(), g->color.blueF(), 1.0);
					switch (g->get_type()) {
					case GIZMO_TYPE_DOT: // draw dot
						glBegin(GL_QUADS);
						glVertex3f(g->screen_pos[0].x()-dot_size, g->screen_pos[0].y()-dot_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x()+dot_size, g->screen_pos[0].y()-dot_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x()+dot_size, g->screen_pos[0].y()+dot_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x()-dot_size, g->screen_pos[0].y()+dot_size, gizmo_z);
						glEnd();
						break;
					case GIZMO_TYPE_POLY: // draw lines
						glBegin(GL_LINES);
						for (int k=1;k<g->get_point_count();k++) {
							glVertex3f(g->screen_pos[k-1].x(), g->screen_pos[k-1].y(), gizmo_z);
							glVertex3f(g->screen_pos[k].x(), g->screen_pos[k].y(), gizmo_z);
						}
						glVertex3f(g->screen_pos[g->get_point_count()-1].x(), g->screen_pos[g->get_point_count()-1].y(), gizmo_z);
						glVertex3f(g->screen_pos[0].x(), g->screen_pos[0].y(), gizmo_z);
						glEnd();
						break;
					case GIZMO_TYPE_TARGET: // draw target
						glBegin(GL_LINES);
						glVertex3f(g->screen_pos[0].x()-target_size, g->screen_pos[0].y()-target_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x()+target_size, g->screen_pos[0].y()-target_size, gizmo_z);

						glVertex3f(g->screen_pos[0].x()+target_size, g->screen_pos[0].y()-target_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x()+target_size, g->screen_pos[0].y()+target_size, gizmo_z);

						glVertex3f(g->screen_pos[0].x()+target_size, g->screen_pos[0].y()+target_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x()-target_size, g->screen_pos[0].y()+target_size, gizmo_z);

						glVertex3f(g->screen_pos[0].x()-target_size, g->screen_pos[0].y()+target_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x()-target_size, g->screen_pos[0].y()-target_size, gizmo_z);

						glVertex3f(g->screen_pos[0].x()-target_size, g->screen_pos[0].y(), gizmo_z);
						glVertex3f(g->screen_pos[0].x()+target_size, g->screen_pos[0].y(), gizmo_z);

						glVertex3f(g->screen_pos[0].x(), g->screen_pos[0].y()-target_size, gizmo_z);
						glVertex3f(g->screen_pos[0].x(), g->screen_pos[0].y()+target_size, gizmo_z);
						glEnd();
						break;
					}
				}
				glPopMatrix();

				glColor4f(color[0], color[1], color[2], color[3]);

				drawn_gizmos = true;
			}

			glDisable(GL_DEPTH);
			glDisable(GL_BLEND);
			glDisable(GL_TEXTURE_2D);
		} while (loop);
	}
}
