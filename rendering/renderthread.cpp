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
#include <QDateTime>
#include <QFileInfo>
#include <QOpenGLExtraFunctions>
#include <QDebug>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "timeline/sequence.h"
#include "effects/effectloaders.h"
#include "global/config.h"
#include "rendering/renderfunctions.h"
#include "rendering/shadergenerators.h"

RenderThread::RenderThread() :
  gizmos(nullptr),
  share_ctx(nullptr),
  ctx(nullptr),
  blend_mode_program(nullptr),
  seq(nullptr),
  tex_width(-1),
  tex_height(-1),
  queued(false),
  texture_failed(false),
  ocio_lut_texture(0),
  ocio_shader(nullptr),
  running(true),
  ocio_config_date(0),
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
        if (!composite_buffer.IsCreated()) {
          composite_buffer.Create(ctx, seq->width, seq->height);
        }
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

        // If there's no blending mode shader, create it now
        if (blend_mode_program == nullptr) {
          delete_shaders();

          blend_mode_program = std::make_shared<QOpenGLShaderProgram>();
          blend_mode_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/internalshaders/common.vert");
          olive::effects_loaded.lock();
          blend_mode_program->addShaderFromSourceCode(QOpenGLShader::Fragment, olive::generated_blending_shader);
          olive::effects_loaded.unlock();
          blend_mode_program->link();

          pipeline_program = olive::shader::GetPipeline();
        }

        // If there's no OpenColorIO shader or the configuration has changed, (re-)create it now
        if (olive::CurrentConfig.enable_color_management && ocio_shader == nullptr) {
          destroy_ocio();

          set_up_ocio();
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

  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  // Get current OCIO display from Config (or defaults if there is no setting)
  QString display = olive::CurrentConfig.ocio_display;
  if (display.isEmpty()) {
    display = config->getDefaultDisplay();
  }

  QString view = olive::CurrentConfig.ocio_view;
  if (view.isEmpty()) {
    view = config->getDefaultView(display.toUtf8());
  }

  // Get current display stats
  OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
  transform->setInputColorSpaceName(OCIO::ROLE_SCENE_LINEAR);
  transform->setDisplay(display.toUtf8());
  transform->setView(view.toUtf8());

  if (!olive::CurrentConfig.ocio_look.isEmpty()) {
    transform->setLooksOverride(olive::CurrentConfig.ocio_look.toUtf8());
    transform->setLooksOverrideEnabled(true);
  }

  try {

    // Using the current configuration, try to get an OCIO processor with a corresponding input and output colorspace
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);

    // Create a OCIO shader with this processor
    ocio_shader = olive::shader::SetupOCIO(ctx, ocio_lut_texture, processor, true);

  } catch(OCIO::Exception & e) {
    qCritical() << e.what();
    return;
  }
}

void RenderThread::destroy_ocio()
{
  // Destroy LUT texture
  if (ocio_lut_texture > 0) {
    ctx->functions()->glDeleteTextures(1, &ocio_lut_texture);
  }
  ocio_lut_texture = 0;
  ocio_shader = nullptr;
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
  params.blend_mode_program = blend_mode_program.get();
  params.pipeline = pipeline_program.get();
  params.backend_buffer1 = &back_buffer_1;
  params.backend_buffer2 = &back_buffer_2;
  params.main_buffer = &composite_buffer;

  // get currently selected gizmos
  gizmos = seq->GetSelectedGizmo();
  params.gizmos = gizmos;

  QOpenGLFunctions* f = ctx->functions();

  f->glEnable(GL_BLEND);

  // bind composite framebuffer for drawing
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, composite_buffer.buffer());

  // Clear framebuffer to nothing
  f->glClearColor(0.0, 0.0, 0.0, 0.0);
  f->glClear(GL_COLOR_BUFFER_BIT);

  // Compose the current frame
  olive::rendering::compose_sequence(params);

  // Copy composite buffer to front buffer
  // First lock the appropriate mutex for exclusivity
  QMutex& active_mutex = front_buffer_switcher ? front_mutex1 : front_mutex2;
  active_mutex.lock();

  FramebufferObject& buffer = front_buffer_switcher ? front_buffer_1 : front_buffer_2;

  // Blit the composite buffer to one of the front buffers

  // If we're color managing, conver the linear composited frame to display color space
  if (olive::CurrentConfig.enable_color_management && ocio_shader != nullptr) {

    olive::rendering::OCIOBlit(ocio_shader.get(),
                               ocio_lut_texture,
                               buffer,
                               composite_buffer.texture());

  } else {

    // If we're not color managing, just blit normally
    buffer.BindBuffer();
    composite_buffer.BindTexture();
    olive::rendering::Blit(pipeline_program.get());
    composite_buffer.ReleaseTexture();
    buffer.ReleaseBuffer();

  }

  // flush changes
  f->glFinish();

  f->glDisable(GL_BLEND);

  texture_failed = params.texture_failed;

  active_mutex.unlock();

  if (!save_fn.isEmpty()) {
    if (texture_failed) {
      // texture failed, try again
      queued = true;
    } else {
      f->glBindFramebuffer(GL_READ_FRAMEBUFFER, composite_buffer.buffer());
      QImage img(tex_width, tex_height, QImage::Format_RGBA8888);
      f->glReadPixels(0, 0, tex_width, tex_height, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
      img.save(save_fn);
      f->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      save_fn = "";
    }
  }

  if (pixel_buffer != nullptr) {

    // set main framebuffer to the current read buffer
    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, composite_buffer.buffer());

    // store pixels in buffer
    f->glReadPixels(0,
                    0,
                    pixel_buffer_linesize == 0 ? tex_width : pixel_buffer_linesize,
                    tex_height,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    pixel_buffer);

    // release current read buffer
    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    pixel_buffer = nullptr;
  }

  // release
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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

void RenderThread::wait_until_paused()
{

  // Wait for thread to finish whatever it's doing before proceeding.
  //
  // FIXME: This is slow. Perhaps there's a better way...

  if (wait_lock_.tryLock()) {
    wait_lock_.unlock();
    return;
  } else {
    wait_lock_.lock();
    wait_lock_.unlock();
  }
}

void RenderThread::delete_buffers() {
  composite_buffer.Destroy();
  front_buffer_1.Destroy();
  front_buffer_2.Destroy();
  back_buffer_1.Destroy();
  back_buffer_2.Destroy();
}

void RenderThread::delete_shaders() {
  blend_mode_program = nullptr;
  pipeline_program = nullptr;
}

void RenderThread::delete_ctx() {
  if (ctx != nullptr) {
    delete_shaders();
    delete_buffers();
    destroy_ocio();
  }

  delete ctx;
  ctx = nullptr;
}
