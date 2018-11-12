#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QOpenGLWidget>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class Viewer;
struct Clip;
struct MediaStream;
class QOpenGLFramebufferObject;
class Effect;
struct GLTextureCoords;

class ViewerWidget : public QOpenGLWidget
{
	Q_OBJECT
public:
	ViewerWidget(QWidget *parent = 0);

    void paintGL();
    void initializeGL();
	Viewer* viewer;

    QOpenGLFramebufferObject* default_fbo;

	bool waveform;
	Clip* waveform_clip;
	MediaStream* waveform_ms;
protected:
    void paintEvent(QPaintEvent *e);
//    void resizeGL(int w, int h);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
private:
	QTimer retry_timer;
    void drawTitleSafeArea();
	bool dragging;
	void seek_from_click(int x);
	GLuint compose_sequence(QVector<Clip *> &nests, bool render_audio);
	GLuint draw_clip(QOpenGLFramebufferObject *clip, GLuint texture);
	void process_effect(Clip* c, Effect* e, double timecode, GLTextureCoords& coords, GLuint& composite_texture, bool& fbo_switcher);
private slots:
	void retry();
    void deleteFunction();
	void show_context_menu();
	void save_frame();
};

#endif // VIEWERWIDGET_H
