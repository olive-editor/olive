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

#ifndef RENDERERPROCESSTHREAD_H
#define RENDERERPROCESSTHREAD_H

#include "rendererthreadbase.h"

class RendererProcessor;

class RendererProcessThread : public RendererThreadBase
{
  Q_OBJECT
public:
  RendererProcessThread(RendererProcessor* parent,
                        QOpenGLContext* share_ctx,
                        const int& width,
                        const int& height,
                        const olive::PixelFormat& format,
                        const olive::RenderMode& mode);

  bool Queue(const NodeDependency &dep, bool wait);

  const QByteArray& hash();

  RenderTexturePtr texture();

public slots:
  virtual void Cancel() override;

protected:
  virtual void ProcessLoop() override;

signals:
  void RequestSibling(NodeDependency dep);

  void FinishedPath();

private:
  RendererProcessor* parent_;

  NodeDependency path_;

  rational time_;

  QByteArray hash_;

  RenderTexturePtr texture_;

  QAtomicInt cancelled_;

};

using RendererProcessThreadPtr = std::shared_ptr<RendererProcessThread>;

#endif // RENDERERPROCESSTHREAD_H
