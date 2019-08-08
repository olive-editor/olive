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

#ifndef RENDERER_H
#define RENDERER_H

#include "node/node.h"
#include "rendererthread.h"

/**
 * @brief A multithreaded OpenGL based renderer for node systems
 */
class RendererProcessor : public Node
{
  Q_OBJECT
public:
  /**
   * @brief Renderer Constructor
   *
   * Constructing a Renderer object will not start any threads/backend on its own. Use Start() to do this and Stop()
   * when the Renderer is about to be destroyed.
   */
  RendererProcessor();

  virtual QString Name() override;
  virtual QString Category() override;
  virtual QString Description() override;

  /**
   * @brief Set parameters of the Renderer
   *
   * The Renderer owns the buffers that are used in the rendering process and this function sets the kind of buffers
   * to use. The Renderer must be stopped when calling this function.
   *
   * @param width
   *
   * Buffer width
   *
   * @param height
   *
   * Buffer height
   *
   * @param format
   *
   * Buffer pixel format
   */
  void SetParameters(const int& width, const int& height, const olive::PixelFormat& format);

  /**
   * @brief Allocate and start the multithreaded backend
   */
  void Start();

  /**
   * @brief Terminate and deallocate the multithreaded backend
   */
  void Stop();

  /**
   * @brief Return current instance of a RenderThread (or nullptr if there is none)
   *
   * This function attempts a dynamic_cast on QThread::currentThread() to RendererThread, which will return nullptr if
   * the cast fails (e.g. if this function is called from the main thread rather than a RendererThread).
   */
  static RendererThread *CurrentThread();

private:
  QVector<RendererThreadPtr> threads_;

  bool started_;
};

#endif // RENDERER_H
