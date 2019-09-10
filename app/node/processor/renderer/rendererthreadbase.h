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

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <memory>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "node/node.h"
#include "render/renderinstance.h"

class RendererThreadBase : public QThread
{
  Q_OBJECT
public:
  RendererThreadBase(QOpenGLContext* share_ctx,
                     const int& width,
                     const int& height,
                     const int& divider,
                     const olive::PixelFormat& format,
                     const olive::RenderMode& mode);

  RenderInstance* render_instance();

  void StartThread(Priority priority = InheritPriority);

  virtual void run() override;

public slots:
  virtual void Cancel() = 0;

protected:
  virtual void ProcessLoop() = 0;

  QWaitCondition wait_cond_;

  QMutex mutex_;

  QMutex caller_mutex_;

private:
  QOpenGLContext* share_ctx_;

  const int& width_;

  const int& height_;

  const int& divider_;

  const olive::PixelFormat& format_;

  const olive::RenderMode& mode_;

  RenderInstance* render_instance_;

};

using RendererThreadPtr = std::shared_ptr<RendererThreadBase>;

#endif // RENDERTHREAD_H
