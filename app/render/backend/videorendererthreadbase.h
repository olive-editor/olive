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
#include <OpenImageIO/imageio.h>

#include "node/node.h"
#include "render/videoparams.h"
#include "renderinstance.h"
#include "render/pixelservice.h"

class VideoRenderBackend;

class VideoRendererThreadBase : public QObject
{
  Q_OBJECT
public:
  VideoRendererThreadBase(VideoRenderBackend* parent, QOpenGLContext* share_ctx, const VideoRenderingParams& params);

  RenderInstance* render_instance();

public slots:
  void Start();

  void Stop();

  void Process(const NodeDependency &dep, bool sibling);

  void Download(RenderTexturePtr texture, const QString &fn, const QByteArray &hash);

signals:
  void RequestSibling(NodeDependency dep);

  void CachedFrame(RenderTexturePtr texture, const rational& time, const QByteArray& hash);

  void FrameSkipped(const rational& time, const QByteArray& hash);

  void Downloaded(const QByteArray& hash);

private:
  void WakeCaller();

  VideoRenderBackend* parent_;

  QOpenGLContext* share_ctx_;

  RenderInstance render_instance_;

  RenderTexturePtr texture_;

  GLuint read_buffer_;

  QOpenGLFunctions* f;
  QOpenGLExtraFunctions* xf;

  PixelFormatInfo format_info_;
  OIIO::ImageSpec spec_;

  QVector<uchar> data_buffer_;

};

using RendererThreadPtr = std::shared_ptr<VideoRendererThreadBase>;

#endif // RENDERTHREAD_H
