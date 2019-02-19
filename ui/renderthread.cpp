/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "renderthread.h"

#include <QApplication>
#include <QImage>
#include <QOpenGLFunctions>
#include <QDebug>

#include "ui/renderfunctions.h"
#include "playback/playback.h"
#include "project/sequence.h"

RenderThread::RenderThread() :
	front_buffer(0),
	front_texture(0),
	gizmos(nullptr),
	share_ctx(nullptr),
	ctx(nullptr),
	blend_mode_program(nullptr),
	premultiply_program(nullptr),
	seq(nullptr),
	tex_width(-1),
	tex_height(-1),
	queued(false),
	texture_failed(false),
	running(true)
{
	surface.create();
}

RenderThread::~RenderThread() {
	surface.destroy();
}

void RenderThread::run() {
	mutex.lock();

	while (running) {
		if (!queued) {
			waitCond.wait(&mutex);
		}
		if (!running) {
			break;
		}
		queued = false;

		if (share_ctx != nullptr) {
			if (ctx != nullptr) {
				ctx->makeCurrent(&surface);

				// gen fbo
				if (front_buffer == 0) {
					// delete any existing framebuffers
					delete_fbo();

					// create framebuffers
					ctx->functions()->glGenFramebuffers(1, &front_buffer);
					ctx->functions()->glGenFramebuffers(1, &back_buffer_1);
					ctx->functions()->glGenFramebuffers(1, &back_buffer_2);
				}

				// gen texture
				if (front_texture == 0 || tex_width != seq->width || tex_height != seq->height) {
					// cache texture size
					tex_width = seq->width;
					tex_height = seq->height;

					// delete any existing textures
					delete_texture();

					// create texture
					glGenTextures(1, &front_texture);
					glGenTextures(1, &back_texture_1);
					glGenTextures(1, &back_texture_2);

					GLuint fbos[3] = {front_buffer, back_buffer_1, back_buffer_2};
					GLuint textures[3] = {front_texture, back_texture_1, back_texture_2};

					for (int i=0;i<3;i++) {
						// bind framebuffer for attaching
						ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[i]);

						// bind texture
						glBindTexture(GL_TEXTURE_2D, textures[i]);

						// allocate storage for texture
						glTexImage2D(
							GL_TEXTURE_2D, 0, GL_RGBA, seq->width, seq->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
						);

						// set texture filtering to bilinear
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

						// attach texture to framebuffer
						ctx->functions()->glFramebufferTexture2D(
							GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0
						);

						// release texture
						glBindTexture(GL_TEXTURE_2D, 0);

						// release framebuffer
						ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
					}
				}

				if (blend_mode_program == nullptr) {
					// create shader program to make blending modes work
					delete_shader_program();

					blend_mode_program = new QOpenGLShaderProgram();
					blend_mode_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/internalshaders/common.vert");
					blend_mode_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/internalshaders/blending.frag");
					blend_mode_program->link();

					premultiply_program = new QOpenGLShaderProgram();
					premultiply_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/internalshaders/common.vert");
					premultiply_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/internalshaders/premultiply.frag");
					premultiply_program->link();
				}

				// bind framebuffer for drawing
				ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, front_buffer);

				// draw frame
				paint();

				// flush changes
				ctx->functions()->glFinish();

				// release
				ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

				emit ready();
			}
		}
	}

	delete_ctx();

	mutex.unlock();
}

void RenderThread::paint() {
	glLoadIdentity();

    glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH);

	gizmos = nullptr;

	ComposeSequenceParams params;
	params.viewer = nullptr;
	params.ctx = ctx;
	params.seq = seq;
	params.video = true;
    params.texture_failed = false;
	params.gizmos = &gizmos;
    params.single_threaded = false;
	params.playback_speed = 1;
	params.blend_mode_program = blend_mode_program;
	params.premultiply_program = premultiply_program;
	params.backend_buffer1 = back_buffer_1;
	params.backend_buffer2 = back_buffer_2;
	params.backend_attachment1 = back_texture_1;
	params.backend_attachment2 = back_texture_2;
	params.main_buffer = front_buffer;
	params.main_attachment = front_texture;
	compose_sequence(params);

	texture_failed = params.texture_failed;

	if (!save_fn.isEmpty()) {
		if (texture_failed) {
			// texture failed, try again
			queued = true;
		} else {
			ctx->functions()->glBindFramebuffer(GL_READ_FRAMEBUFFER, front_buffer);
			QImage img(tex_width, tex_height, QImage::Format_RGBA8888);
			glReadPixels(0, 0, tex_width, tex_height, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
			img.save(save_fn);
			ctx->functions()->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			save_fn = "";
		}
	}

	if (pixel_buffer != nullptr) {

        // set main framebuffer to the current read buffer
        ctx->functions()->glBindFramebuffer(GL_READ_FRAMEBUFFER, front_buffer);

        // store pixels in buffer
        glReadPixels(0,
                     0,
                     pixel_buffer_linesize == 0 ? tex_width : pixel_buffer_linesize,
                     tex_height,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     pixel_buffer);

        // release current read buffer
		ctx->functions()->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

		pixel_buffer = nullptr;
	}

	glDisable(GL_DEPTH);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void RenderThread::start_render(QOpenGLContext *share, Sequence *s, const QString& save, GLvoid* pixels, int pixel_linesize, int idivider) {
    Q_UNUSED(idivider);

	seq = s;

	// stall any dependent actions
	texture_failed = true;

	if (share != nullptr && (ctx == nullptr || ctx->shareContext() != share_ctx)) {
		share_ctx = share;
		delete_ctx();
		ctx = new QOpenGLContext();
		ctx->setFormat(share_ctx->format());
		ctx->setShareContext(share_ctx);
		ctx->create();
		ctx->moveToThread(this);
	}

	save_fn = save;
	pixel_buffer = pixels;
    pixel_buffer_linesize = pixel_linesize;

	queued = true;

	waitCond.wakeAll();
}

bool RenderThread::did_texture_fail() {
	return texture_failed;
}

void RenderThread::cancel() {
	running = false;
	waitCond.wakeAll();
	wait();
}

void RenderThread::delete_texture() {
	if (front_texture > 0) {
		GLuint tex[3] = {front_texture, back_texture_1, back_texture_2};
		glDeleteTextures(3, tex);
	}
	front_texture = 0;
	back_texture_1 = 0;
	back_texture_2 = 0;
}

void RenderThread::delete_fbo() {
	if (front_buffer > 0) {
		GLuint fbos[3] = {front_buffer, back_buffer_1, back_buffer_2};
		ctx->functions()->glDeleteFramebuffers(3, fbos);
	}
	front_buffer = 0;
	back_buffer_1 = 0;
	back_buffer_2 = 0;
}

void RenderThread::delete_shader_program() {
	if (blend_mode_program != nullptr) {
		delete blend_mode_program;
		delete premultiply_program;
	}
	blend_mode_program = nullptr;
	premultiply_program = nullptr;
}

void RenderThread::delete_ctx() {
	if (ctx != nullptr) {
		delete_shader_program();
		delete_texture();
		delete_fbo();
		ctx->doneCurrent();
		delete ctx;
	}
	ctx = nullptr;
}
