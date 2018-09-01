#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QOpenGLWidget>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

struct Clip;
struct Sequence;
class QOpenGLFramebufferObject;

class ViewerWidget : public QOpenGLWidget
{
	Q_OBJECT
public:
	ViewerWidget(QWidget *parent = 0);

	bool rendering;
    void paintGL();
    void initializeGL();

	QOpenGLFramebufferObject* fbo;
protected:
    void paintEvent(QPaintEvent *e);
//    void resizeGL(int w, int h);
private:
	QTimer retry_timer;
private slots:
	void retry();
    void deleteFunction();
	GLuint compose_sequence(Clip *nest, bool render_audio);
	GLuint draw_clip(Clip *clip, GLuint texture);
};

#endif // VIEWERWIDGET_H
