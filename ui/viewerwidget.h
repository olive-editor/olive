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

class Viewer;
struct Clip;
struct FootageStream;
class QOpenGLFramebufferObject;
class Effect;
class EffectGizmo;
class ViewerContainer;
struct GLTextureCoords;
class RenderThread;
class ViewerWindow;

class ViewerWidget : public QOpenGLWidget, QOpenGLFunctions
{
	Q_OBJECT
public:
	ViewerWidget(QWidget *parent = 0);
	~ViewerWidget();

	void delete_function();

	void paintGL();
	void initializeGL();
	Viewer* viewer;
	ViewerContainer* container;

	QOpenGLFramebufferObject* default_fbo;

	bool waveform;
	Clip* waveform_clip;
	const FootageStream* waveform_ms;
	double waveform_zoom;
	int waveform_scroll;

	bool force_quit;

	void frame_update();
public slots:
	void set_waveform_scroll(int s);
protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
private:
	void drawTitleSafeArea();
	bool dragging;
	void seek_from_click(int x);
	Effect* gizmos;
	int drag_start_x;
	int drag_start_y;
	int gizmo_x_mvmt;
	int gizmo_y_mvmt;
	EffectGizmo* selected_gizmo;
	EffectGizmo* get_gizmo_from_mouse(int x, int y);
	bool drawn_gizmos;
	void move_gizmos(QMouseEvent *event, bool done);
	RenderThread* renderer;
	ViewerWindow* window;
private slots:
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
