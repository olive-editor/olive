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

#include <QMutex>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QThread>
#include <QWaitCondition>

#include "node/node.h"
#include "render/texturebuffer.h"

class RendererThread : public QThread
{
public:
  RendererThread();

  QOpenGLContext* context();

  TextureBuffer* buffer();

  bool Queue(Node* n, const rational &time);

  void Cancel();

  virtual void run() override;

private:
  QOpenGLContext ctx_;

  QOffscreenSurface surface_;

  TextureBuffer buffer_;

  QWaitCondition wait_cond_;

  QMutex mutex_;

  QMutex caller_mutex_;

  Node* node_;

  rational time_;

  bool cancelled_;
};

#endif // RENDERTHREAD_H
