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
#include "ui/renderfunctions.h"
#include "ui/renderthread.h"

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

ViewerWidget::ViewerWidget(QWidget *parent) :
	QOpenGLWidget(parent),
	default_fbo(nullptr),
	waveform(false),
	waveform_zoom(1.0),
	waveform_scroll(0),
	dragging(false),
	gizmos(nullptr),
	selected_gizmo(nullptr)
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

	renderer = new RenderThread();
	renderer->start(QThread::HighPriority);
}

void ViewerWidget::delete_function() {
	// destroy all textures as well
	if (viewer->seq != nullptr) {
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

	QAction* save_frame_as_image = menu.addAction(tr("Save Frame as Image..."));
	connect(save_frame_as_image, SIGNAL(triggered(bool)), this, SLOT(save_frame()));

	QAction* show_fullscreen_action = menu.addAction(tr("Show Fullscreen"));
	connect(show_fullscreen_action, SIGNAL(triggered()), this, SLOT(show_fullscreen()));

	QMenu zoom_menu(tr("Zoom"));
	QAction* fit_zoom = zoom_menu.addAction(tr("Fit"));
	connect(fit_zoom, SIGNAL(triggered(bool)), this, SLOT(set_fit_zoom()));
	zoom_menu.addAction("10%")->setData(0.1);
	zoom_menu.addAction("25%")->setData(0.25);
	zoom_menu.addAction("50%")->setData(0.5);
	zoom_menu.addAction("75%")->setData(0.75);
	zoom_menu.addAction("100%")->setData(1.0);
	zoom_menu.addAction("150%")->setData(1.5);
	zoom_menu.addAction("200%")->setData(2.0);
	zoom_menu.addAction("400%")->setData(4.0);
	QAction* custom_zoom = zoom_menu.addAction(tr("Custom"));
	connect(custom_zoom, SIGNAL(triggered(bool)), this, SLOT(set_custom_zoom()));
	connect(&zoom_menu, SIGNAL(triggered(QAction*)), this, SLOT(set_menu_zoom(QAction*)));
	menu.addMenu(&zoom_menu);

	if (!viewer->is_main_sequence()) {
		menu.addAction(tr("Close Media"), viewer, SLOT(close_media()));
	}

	menu.exec(QCursor::pos());
}

void ViewerWidget::save_frame() {
	QFileDialog fd(this);
	fd.setAcceptMode(QFileDialog::AcceptSave);
	fd.setFileMode(QFileDialog::AnyFile);
	fd.setWindowTitle(tr("Save Frame"));
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
		default_fbo = nullptr;
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
	double d = QInputDialog::getDouble(this,
									   tr("Viewer Zoom"),
									   tr("Set Custom Zoom Value:"),
									   container->zoom*100, 0, 2147483647, 2, &ok);
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
	if (gizmos != nullptr) {
		double multiplier = double(viewer->seq->width) / double(width());
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
	return nullptr;
}

void ViewerWidget::move_gizmos(QMouseEvent *event, bool done) {
	if (selected_gizmo != nullptr) {
		double multiplier = double(viewer->seq->width) / double(width());

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

		if (selected_gizmo != nullptr) {
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
		} else if (gizmos == nullptr) {
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
		if (g != nullptr) {
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

void ViewerWidget::paintGL() {
	if (viewer->seq != nullptr) {
		gizmos = nullptr;
		drawn_gizmos = false;
		force_quit = false;

		bool render_audio = (viewer->playing || rendering);

		retry_timer.stop();

		// send context to other thread for drawing
		doneCurrent();
		renderer->start_render(context(), viewer->seq);

		// render the audio
		QVector<Clip*> nests;
		compose_sequence(viewer, context(), viewer->seq, nests, false, render_audio, &gizmos);

		// try to draw the texture from the other thread if we got it
		if (renderer->mutex.tryLock()) {
			makeCurrent();

			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);

			glEnable(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, renderer->texColorBuffer);

			glLoadIdentity();
			glOrtho(0, 1, 1, 0, -1, 1);

			glBegin(GL_QUADS);

			glVertex2f(0, 0);
			glTexCoord2f(0, 0);
			glVertex2f(0, 1);
			glTexCoord2f(1, 0);
			glVertex2f(1, 1);
			glTexCoord2f(1, 1);
			glVertex2f(1, 0);
			glTexCoord2f(0, 1);

			glEnd();

			glBindTexture(GL_TEXTURE_2D, 0);

			glDisable(GL_TEXTURE_2D);

			renderer->mutex.unlock();
		}
	}


	/*drawn_gizmos = false;
	force_quit = false;
	if (viewer->seq != nullptr) {
		gizmos = nullptr;

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
					qInfo() << "Texture failed - looping";
					loop = true;
				} else if (!viewer->playing) {
					retry_timer.start();
				}
			}

			if (config.show_title_safe_area && !rendering) {
				drawTitleSafeArea();
			}

			if (gizmos != nullptr && drawn_gizmos) {
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
	}*/
}
