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

#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QOpenGLWidget>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QOpenGLFunctions>

#include "project/clip.h"
#include "project/footage.h"
#include "project/effect.h"
#include "ui/viewerwindow.h"
#include "ui/viewercontainer.h"
#include "ui/renderthread.h"

class Viewer;
class QOpenGLFramebufferObject;
struct GLTextureCoords;

class ViewerWidget : public QOpenGLWidget, QOpenGLFunctions
{
	Q_OBJECT
public:
	ViewerWidget(QWidget *parent = nullptr);
	~ViewerWidget();

	void delete_function();
	void close_window();

	void paintGL();
	void initializeGL();
	Viewer* viewer;
	ViewerContainer* container;

	bool waveform;
    ClipPtr waveform_clip;
	const FootageStream* waveform_ms;
	double waveform_zoom;
	int waveform_scroll;

	void frame_update();
	RenderThread* get_renderer();
	void set_scroll(double x, double y);
public slots:
	void set_waveform_scroll(int s);
    void set_fullscreen(int screen = 0);
protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent* event);
private:
	void draw_waveform_func();
	void draw_title_safe_area();
	void draw_gizmos();
	EffectGizmo* get_gizmo_from_mouse(int x, int y);
	void move_gizmos(QMouseEvent *event, bool done);
	bool dragging;
	void seek_from_click(int x);
    EffectPtr gizmos;
	int drag_start_x;
	int drag_start_y;
	int gizmo_x_mvmt;
	int gizmo_y_mvmt;
	EffectGizmo* selected_gizmo;
	RenderThread* renderer;
	ViewerWindow* window;
	double x_scroll;
	double y_scroll;
private slots:
	void context_destroy();
	void retry();
	void show_context_menu();
	void save_frame();
	void queue_repaint();
	void fullscreen_menu_action(QAction* action);
	void set_fit_zoom();
	void set_custom_zoom();
	void set_menu_zoom(QAction *action);
};

#endif // VIEWERWIDGET_H
