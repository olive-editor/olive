/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef GLINSTANCE_H
#define GLINSTANCE_H

#include <QMatrix4x4>
#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "opengl/openglshader.h"
#include "opengl/openglframebuffer.h"
#include "render/rendermodes.h"
#include "render/videoparams.h"

/**
 * @brief An object containing all resources necessary for each thread to support hardware accelerated rendering
 *
 * RenderInstance contains everything that Nodes will need to draw with on a per-thread basis.
 *
 * Due to its usage of QOffscreenSurface, a RenderInstance instance must be constructed in the main (GUI) thread. From
 * there it is safe to call Start() on in another thread.
 */
class RenderInstance : public QObject
{
public:
  RenderInstance(const VideoRenderingParams &params);

  virtual ~RenderInstance() override;

  Q_DISABLE_COPY_MOVE(RenderInstance)

  void SetShareContext(QOpenGLContext* share);

  bool Start();

  void Stop();

  bool IsStarted();

  OpenGLFramebuffer* buffer();

  QOpenGLContext* context();

  const VideoRenderingParams& params() const;

  OpenGLShaderPtr default_pipeline() const;

private:
  QOpenGLContext* ctx_;

  QOpenGLContext* share_ctx_;

  QOffscreenSurface surface_;

  OpenGLFramebuffer buffer_;

  VideoRenderingParams params_;

  OpenGLShaderPtr default_pipeline_;
};

#endif // GLINSTANCE_H
