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

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>

#include "timeline/sequence.h"
#include "nodes/oldeffectnode.h"
#include "rendering/framebufferobject.h"
#include "qopenglshaderprogramptr.h"

class RenderThread : public QThread {
  Q_OBJECT
public:
  RenderThread();
  ~RenderThread();
  void run();

  QMutex* get_texture_mutex();
  const GLuint& get_texture();

  OldEffectNode* gizmos;
  void paint();
  void start_render(QOpenGLContext* share,
                    Sequence *s,
                    int playback_speed,
                    const QString &save = nullptr,
                    GLvoid *pixels = nullptr,
                    int pixel_linesize = 0,
                    int idivider = 0);
  bool did_texture_fail();
  void cancel();
  void wait_until_paused();

public slots:
  // cleanup functions
  void delete_ctx();
  void delete_buffers();
  void delete_shaders();
  void destroy_ocio();
signals:
  void ready();
private:

  // OpenColorIO functions
  void set_up_ocio();

  // OpenColorIO variables
  GLuint ocio_lut_texture;
  QOpenGLShaderProgramPtr ocio_shader;
  qint64 ocio_config_date;

  FramebufferObject front_buffer_1;
  QMutex front_mutex1;

  FramebufferObject front_buffer_2;
  QMutex front_mutex2;

  FramebufferObject composite_buffer;

  bool front_buffer_switcher;

  QWaitCondition wait_cond_;
  QMutex wait_lock_;

  QWaitCondition main_thread_wait_cond_;
  QMutex main_thread_lock_;

  QOffscreenSurface surface;
  QOpenGLContext* share_ctx;
  QOpenGLContext* ctx;
  QOpenGLShaderProgramPtr pipeline_program;

  FramebufferObject back_buffer_1;
  FramebufferObject back_buffer_2;

  Sequence* seq;

  int playback_speed_;
  int divider;
  int tex_width;
  int tex_height;
  bool queued;
  bool texture_failed;
  bool running;
  QString save_fn;
  GLvoid *pixel_buffer;
  int pixel_buffer_linesize;
};

#endif // RENDERTHREAD_H
