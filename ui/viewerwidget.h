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

class AudioSenderThread : public QThread {
public:
	AudioSenderThread();
	void run();
	QWaitCondition cond;
	bool close;
	QMutex lock;
private:
	QVector<qint16> samples;
	int send_audio_to_output(int offset, int max);
};

class ViewerWidget : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT
public:
    ViewerWidget(QWidget *parent = 0);
    ~ViewerWidget();

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
	AudioSenderThread audio_sender_thread;
private slots:
	void retry();
    void deleteFunction();
	void compose_sequence(QVector<Clip*>& nests, bool render_audio);
};

#endif // VIEWERWIDGET_H
