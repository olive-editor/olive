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

class ViewerWidget : public QOpenGLWidget
{
	Q_OBJECT
public:
	ViewerWidget(QWidget *parent = 0);

    void paintGL();
    void initializeGL();
	Viewer* viewer;

	bool waveform;
	Clip* waveform_clip;
	MediaStream* waveform_ms;

	QOpenGLFramebufferObject* default_fbo;
protected:
    void paintEvent(QPaintEvent *e);
//    void resizeGL(int w, int h);
private:
	QTimer retry_timer;
    void drawTitleSafeArea();
private slots:
	void retry();
    void deleteFunction();
	GLuint compose_sequence(Clip *nest, bool render_audio);
	GLuint draw_clip(QOpenGLFramebufferObject *clip, GLuint texture);
};

#endif // VIEWERWIDGET_H
