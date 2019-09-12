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

#include "render/gl/shaderptr.h"
#include "render/renderframebuffer.h"
#include "render/rendermodes.h"

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
  RenderInstance(const int& width,
                 const int& height,
                 const int& divider,
                 const olive::PixelFormat& format,
                 const olive::RenderMode& mode);

  virtual ~RenderInstance() override;

  /**
   * @brief Deleted copy constructor
   */
  RenderInstance(const RenderInstance& other) = delete;

  /**
   * @brief Deleted move constructor
   */
  RenderInstance(RenderInstance&& other) = delete;

  /**
   * @brief Deleted copy assignment
   */
  RenderInstance& operator=(const RenderInstance& other) = delete;

  /**
   * @brief Deleted move assignment
   */
  RenderInstance& operator=(RenderInstance&& other) = delete;

  void SetShareContext(QOpenGLContext* share);

  bool Start();

  void Stop();

  bool IsStarted();

  RenderFramebuffer* buffer();

  QOpenGLContext* context();

  const int& width() const;

  const int& height() const;

  const int& divider() const;

  const olive::PixelFormat& format() const;

  const olive::RenderMode& mode() const;

  ShaderPtr default_pipeline() const;

private:
  QOpenGLContext* ctx_;

  QOpenGLContext* share_ctx_;

  QOffscreenSurface surface_;

  RenderFramebuffer buffer_;

  int width_;

  int height_;

  olive::PixelFormat format_;

  olive::RenderMode mode_;

  int divider_;

  ShaderPtr default_pipeline_;
};

#endif // GLINSTANCE_H
