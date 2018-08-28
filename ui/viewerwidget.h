#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

struct Clip;
struct Sequence;

class ViewerWidget : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT
public:
	ViewerWidget(QWidget *parent = 0);

    bool multithreaded;
    bool force_audio;
    bool enable_paint;
    bool flip;
    void paintGL();
    void initializeGL();
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
