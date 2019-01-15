#include "renderthread.h"

#include <QApplication>
#include <QImage>
#include <QOpenGLFunctions>
#include <QDebug>

#include "ui/renderfunctions.h"
#include "playback/playback.h"

RenderThread::RenderThread() :
	share_ctx(nullptr),
	ctx(nullptr),
	frameBuffer(0),
	texColorBuffer(0)
{
	surface.create();
}

RenderThread::~RenderThread() {
	surface.destroy();
}

void RenderThread::run() {
	mutex.lock();

	bool running = true;

	while (running) {
		waitCond.wait(&mutex);

		if (share_ctx != nullptr) {
			if (ctx == nullptr) {
				ctx = new QOpenGLContext();
				ctx->setFormat(share_ctx->format());
				ctx->setShareContext(share_ctx);
				ctx->create();
				ctx->makeCurrent(&surface);
			}

			if (ctx != nullptr) {
				// gen fbo
				if (frameBuffer == 0) {
					ctx->functions()->glGenFramebuffers(1, &frameBuffer);
				}

				// bind
				ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);

				// gen texture
				if (texColorBuffer == 0) {
					glGenTextures(1, &texColorBuffer);
					glBindTexture(GL_TEXTURE_2D, texColorBuffer);
					glTexImage2D(
						GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr
					);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					ctx->functions()->glFramebufferTexture2D(
						GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0
					);
					glBindTexture(GL_TEXTURE_2D, 0);
				}

				// draw
				paint();

				// flush changes
				glFlush();

				// release
				ctx->functions()->glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}
	}

	if (ctx != nullptr) {
		if (frameBuffer != 0) ctx->functions()->glDeleteFramebuffers(1, &frameBuffer);
		ctx->doneCurrent();
		delete ctx;
	}

	mutex.unlock();
}

void RenderThread::paint() {
	glLoadIdentity();

	texture_failed = false;

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glClearColor(0, 0, 0, 0);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH);

	Effect* gizmos; // does nothing yet
	QVector<Clip*> nests;

	compose_sequence(nullptr, ctx, seq, nests, true, false, &gizmos);

	glDisable(GL_DEPTH);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void RenderThread::start_render(QOpenGLContext *share, Sequence *s, int idivider) {
	share_ctx = share;
	seq = s;
	waitCond.wakeAll();
}
