#include "renderthread.h"

#include <QApplication>
#include <QImage>
#include <QOpenGLFunctions>
#include <QDebug>

#include "ui/renderfunctions.h"
#include "playback/playback.h"
#include "project/sequence.h"

RenderThread::RenderThread() :
	share_ctx(nullptr),
	ctx(nullptr),
	frameBuffer(0),
	texColorBuffer(0),
	tex_width(-1),
	tex_height(-1),
	queued(false)
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
		if (!queued) {
			waitCond.wait(&mutex);
		}
		queued = false;

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
				if (texColorBuffer == 0 || tex_width != seq->width || tex_height != seq->height) {\
					if (texColorBuffer > 0) {
						ctx->functions()->glFramebufferTexture2D(
							GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0
						);
						glDeleteTextures(1, &texColorBuffer);
					}
					glGenTextures(1, &texColorBuffer);
					glBindTexture(GL_TEXTURE_2D, texColorBuffer);
					glTexImage2D(
						GL_TEXTURE_2D, 0, GL_RGB, seq->width, seq->height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr
					);
					tex_width = seq->width;
					tex_height = seq->height;
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

				emit ready();
			}
		}
	}

	if (ctx != nullptr) {
		if (texColorBuffer > 0) glDeleteTextures(1, &texColorBuffer);
		if (frameBuffer > 0) ctx->functions()->glDeleteFramebuffers(1, &frameBuffer);
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
	queued = true;
	waitCond.wakeAll();
}
