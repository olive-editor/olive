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
public:
	RenderThread();
	~RenderThread();
	void run();
	QMutex mutex;
	GLuint frameBuffer;
	GLuint texColorBuffer;
	void paint();
	void start_render(QOpenGLContext* share, Sequence* s);
private:
	QWaitCondition waitCond;
	QOffscreenSurface surface;
	QOpenGLContext* share_ctx;
	QOpenGLContext* ctx;
	Sequence* seq;
};

#endif // RENDERTHREAD_H
