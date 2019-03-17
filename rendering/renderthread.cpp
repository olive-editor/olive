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
#include <QDebug>

#ifndef NO_OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;
#endif

#include "rendering/renderfunctions.h"
#include "timeline/sequence.h"
#include "effects/effectloaders.h"
#include "global/config.h"

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

        // If there's no blending mode shader, create it now
        if (blend_mode_program == nullptr) {
          delete_shaders();

          blend_mode_program = new QOpenGLShaderProgram();
          blend_mode_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/internalshaders/common.vert");
          olive::effects_loaded.lock();
          blend_mode_program->addShaderFromSourceCode(QOpenGLShader::Fragment, olive::generated_blending_shader);
          olive::effects_loaded.unlock();
          blend_mode_program->link();
        }

#ifndef NO_OCIO
        // If there's no OpenColorIO shader, create it now
        if (olive::CurrentConfig.enable_color_management
            && (ocio_shader == nullptr || ocio_loaded_config != olive::CurrentConfig.ocio_config_path)) {
          destroy_ocio();

          set_up_ocio();
        }
#endif

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

const char * g_fragShaderText = ""
"\n"
"uniform sampler2D tex1;\n"
"uniform sampler3D tex2;\n"
"\n"
"void main()\n"
"{\n"
"    vec4 col = texture2D(tex1, gl_TexCoord[0].st);\n"
"    gl_FragColor = OCIODisplay(col, tex2);\n"
"}\n";

#ifndef NO_OCIO
void RenderThread::set_up_ocio()
{
  functions.initializeOpenGLFunctions();

  //
  // SETUP LUT TEXTURE
  //

  // Create LUT texture
  ctx->functions()->glGenTextures(1, &ocio_lut_texture);

  // Bind texture to GL_TEXTURE_3D and GL_TEXTURE2
  ctx->functions()->glActiveTexture(GL_TEXTURE2);
  ctx->functions()->glBindTexture(GL_TEXTURE_3D, ocio_lut_texture);

  // Set texture parameters
  ctx->functions()->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  ctx->functions()->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  ctx->functions()->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  ctx->functions()->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  ctx->functions()->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // Allocate storage for texture
  functions.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB,
                         OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE,
                         0, GL_RGB,GL_FLOAT, ocio_lut_data);

  //
  // SET UP OCIO DISPLAY
  //

  OCIO::ConstConfigRcPtr config;

  // Set current config to the file specified in Config
  if (QFileInfo::exists(olive::CurrentConfig.ocio_config_path)) {

    config = OCIO::Config::CreateFromFile(olive::CurrentConfig.ocio_config_path.toUtf8());
    OCIO::SetCurrentConfig(config);
    ocio_loaded_config = olive::CurrentConfig.ocio_config_path;

  } else {

    config = OCIO::GetCurrentConfig();

  }

  const char* display = config->getDefaultDisplay();

  OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
  transform->setInputColorSpaceName(OCIO::ROLE_SCENE_LINEAR);
  transform->setDisplay(display);
  transform->setView(config->getDefaultView(display));

  //
  // GET OCIO PROCESSOR
  //

  OCIO::ConstProcessorRcPtr processor;
  try {
    processor = OCIO::GetCurrentConfig()->getProcessor(transform);
  } catch(OCIO::Exception & e) {
    qCritical() << e.what();
    ctx->functions()->glActiveTexture(GL_TEXTURE0);
    return;
  }


  //
  // SET UP GLSL SHADER
  //

  OCIO::GpuShaderDesc shaderDesc;
  shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
  shaderDesc.setFunctionName("OCIODisplay");
  shaderDesc.setLut3DEdgeLen(OCIO_LUT3D_EDGE_SIZE);

  //
  // COMPUTE 3D LUT
  //

  processor->getGpuLut3D(ocio_lut_data, shaderDesc);

  glBindTexture(GL_TEXTURE_3D, ocio_lut_texture);
  functions.glTexSubImage3D(GL_TEXTURE_3D, 0,
                            0, 0, 0,
                            OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE,
                            GL_RGB,GL_FLOAT, ocio_lut_data);

  QString shader_text = processor->getGpuShaderText(shaderDesc);
  shader_text.append("\n");
  shader_text.append(g_fragShaderText);

  ocio_shader = new QOpenGLShaderProgram();
  ocio_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, shader_text);
  ocio_shader->link();

  // Reset active texture to 0 for the rest of the pipeline
  ctx->functions()->glActiveTexture(GL_TEXTURE0);
}

void RenderThread::destroy_ocio()
{
  // Destroy LUT texture
  ctx->functions()->glDeleteTextures(1, &ocio_lut_texture);
  ocio_lut_texture = 0;

  delete ocio_shader;
  ocio_shader = nullptr;
}
#endif

void RenderThread::paint() {
  // set up compose_sequence() parameters
  ComposeSequenceParams params;
  params.viewer = nullptr;
  params.ctx = ctx;
  params.seq = seq;
  params.video = true;
  params.texture_failed = false;
  params.wait_for_mutexes = true;
  params.playback_speed = 1;
  params.blend_mode_program = blend_mode_program;
#ifndef NO_OCIO
  params.ocio_shader = ocio_shader;
#endif
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
                                const QString& save,
                                GLvoid* pixels,
                                int pixel_linesize,
                                int idivider) {
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
}

void RenderThread::delete_ctx() {
  if (ctx != nullptr) {
    delete_shaders();
    delete_buffers();

#ifndef NO_OCIO
    destroy_ocio();
#endif
  }

  delete ctx;
  ctx = nullptr;
}
