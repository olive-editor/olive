#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

struct Sequence;

class RenderThread : public QThread {
	Q_OBJECT
public:
	RenderThread();
	~RenderThread();
	void run();
	QMutex mutex;
	GLuint frameBuffer;
	GLuint texColorBuffer;
	void paint();
	void start_render(QOpenGLContext* share, Sequence* s, int idivider = 0);
signals:
	void ready();
private:
	QWaitCondition waitCond;
	QOffscreenSurface surface;
	QOpenGLContext* share_ctx;
	QOpenGLContext* ctx;
	Sequence* seq;
	int divider;
	int tex_width;
	int tex_height;
	bool queued;
};

#endif // RENDERTHREAD_H
