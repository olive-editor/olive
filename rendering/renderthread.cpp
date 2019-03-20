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
#include <QDateTime>
#include <QDebug>
#ifdef OLIVE_OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;
#endif

#include "rendering/renderfunctions.h"
#include "timeline/sequence.h"

RenderThread::RenderThread() :
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
  running(true),
  front_buffer_switcher(false)
{
  surface.create();
}

RenderThread::~RenderThread() {
  surface.destroy();
}

void RenderThread::run() {
  wait_lock_.lock();

  while (running) {
    if (!queued) {
      wait_cond_.wait(&wait_lock_);
    }
    if (!running) {
      break;
    }
    queued = false;

    if (share_ctx != nullptr) {
      if (ctx != nullptr) {
        ctx->makeCurrent(&surface);

        // if the sequence size has changed, we'll need to reinitialize the textures
        if (seq->width != tex_width || seq->height != tex_height) {
          delete_buffers();

          // cache sequence values for future checks
          tex_width = seq->width;
          tex_height = seq->height;
        }

        // create any buffers that don't yet exist
        if (!front_buffer_1.IsCreated()) {
          front_buffer_1.Create(ctx, seq->width, seq->height);
        }
        if (!front_buffer_2.IsCreated()) {
          front_buffer_2.Create(ctx, seq->width, seq->height);
        }
        if (!back_buffer_1.IsCreated()) {
          back_buffer_1.Create(ctx, seq->width, seq->height);
        }
        if (!back_buffer_2.IsCreated()) {
          back_buffer_2.Create(ctx, seq->width, seq->height);
        }

        if (blend_mode_program == nullptr) {
          // create shader program to make blending modes work
          delete_shaders();

          blend_mode_program = new QOpenGLShaderProgram();
          blend_mode_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/internalshaders/common.vert");
          blend_mode_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/internalshaders/blending.frag");
          blend_mode_program->link();

          premultiply_program = new QOpenGLShaderProgram();
          premultiply_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/internalshaders/common.vert");
          premultiply_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/internalshaders/premultiply.frag");
          premultiply_program->link();
        }

        // draw frame
        paint();

        front_buffer_switcher = !front_buffer_switcher;

        emit ready();
      }
    }
  }

  delete_ctx();

  wait_lock_.unlock();
}

QMutex *RenderThread::get_texture_mutex()
{
  // return the mutex for the opposite texture being drawn to by the renderer
  return front_buffer_switcher ? &front_mutex2 : &front_mutex1;
}

const GLuint &RenderThread::get_texture()
{
  // return the opposite texture to the texture being drawn to by the renderer
  return front_buffer_switcher ? front_buffer_2.texture() : front_buffer_1.texture();
}

void RenderThread::set_up_ocio()
{
}

void RenderThread::paint() {
  // set up compose_sequence() parameters
  ComposeSequenceParams params;
  params.viewer = nullptr;
  params.ctx = ctx;
  params.seq = seq;
  params.video = true;
  params.texture_failed = false;
  params.wait_for_mutexes = true;
  params.playback_speed = playback_speed_;
  params.blend_mode_program = blend_mode_program;
  params.premultiply_program = premultiply_program;
  params.backend_buffer1 = back_buffer_1.buffer();
  params.backend_buffer2 = back_buffer_2.buffer();
  params.backend_attachment1 = back_buffer_1.texture();
  params.backend_attachment2 = back_buffer_2.texture();
  params.main_buffer = front_buffer_switcher ? front_buffer_1.buffer() : front_buffer_2.buffer();
  params.main_attachment = front_buffer_switcher ? front_buffer_1.texture() : front_buffer_2.texture();

  // get currently selected gizmos
  gizmos = seq->GetSelectedGizmo();
  params.gizmos = gizmos;

  QMutex& active_mutex = front_buffer_switcher ? front_mutex1 : front_mutex2;
  active_mutex.lock();

  // bind framebuffer for drawing
  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, params.main_buffer);

  glLoadIdentity();

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  compose_sequence(params);

  // flush changes
  ctx->functions()->glFinish();

  texture_failed = params.texture_failed;

  active_mutex.unlock();

  if (!save_fn.isEmpty()) {
    if (texture_failed) {
      // texture failed, try again
      queued = true;
    } else {
      ctx->functions()->glBindFramebuffer(GL_READ_FRAMEBUFFER, params.main_buffer);
      QImage img(tex_width, tex_height, QImage::Format_RGBA8888);
      glReadPixels(0, 0, tex_width, tex_height, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
      img.save(save_fn);
      ctx->functions()->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      save_fn = "";
    }
  }

  if (pixel_buffer != nullptr) {

    // set main framebuffer to the current read buffer
    ctx->functions()->glBindFramebuffer(GL_READ_FRAMEBUFFER, params.main_buffer);

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

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  // release
  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void RenderThread::start_render(QOpenGLContext *share,
                                Sequence* s,
                                int playback_speed,
                                const QString& save,
                                GLvoid* pixels,
                                int pixel_linesize,
                                int idivider) {
  Q_UNUSED(idivider);

  seq = s;

  playback_speed_ = playback_speed;

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

  wait_cond_.wakeAll();
}

bool RenderThread::did_texture_fail() {
  return texture_failed;
}

void RenderThread::cancel() {
  running = false;
  wait_cond_.wakeAll();
  wait();
}

void RenderThread::delete_buffers() {
  front_buffer_1.Destroy();
  front_buffer_2.Destroy();
  back_buffer_1.Destroy();
  back_buffer_2.Destroy();
}

void RenderThread::delete_shaders() {
  delete blend_mode_program;
  blend_mode_program = nullptr;

  delete premultiply_program;
  premultiply_program = nullptr;
}

void RenderThread::delete_ctx() {
  if (ctx != nullptr) {
    delete_shaders();
    delete_buffers();
  }

  delete ctx;
  ctx = nullptr;
}
