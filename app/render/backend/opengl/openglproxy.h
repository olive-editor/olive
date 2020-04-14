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

#ifndef OPENGLPROXY_H
#define OPENGLPROXY_H

#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "../videorenderworker.h"
#include "openglframebuffer.h"
#include "openglshadercache.h"
#include "opengltexturecache.h"

OLIVE_NAMESPACE_ENTER

class OpenGLProxy : public QObject {
  Q_OBJECT
public:
  OpenGLProxy(QObject* parent = nullptr);

  virtual ~OpenGLProxy() override;

  /**
   * @brief Initialize OpenGL instance in whatever thread this object is a part of
   *
   * This function creates a context (shared with share_ctx provided in the constructor) as well as various other
   * OpenGL thread-specific objects necessary for rendering. This function should only ever be called from the main
   * thread (i.e. the thread where share_ctx is current on) but AFTER this object has been pushed to its thread with
   * moveToThread(). If this function is called from a different thread, it could fail or even segfault on some
   * platforms.
   *
   * The reason this function must be called in the main thread (rather than initializing asynchronously in a separate
   * thread) is because different platforms have different rules about creating a share context with a context that
   * is still "current" in another thread. While some implementations do allow this, Windows OpenGL (wgl) explicitly
   * forbids it and other platforms/drivers will segfault attempting it. While we can obviously call "doneCurrent", I
   * haven't found any reliable way to prevent the main thread from making it current again before initialization is
   * complete other than blocking it entirely.
   *
   * To get around this, we create all share contexts in the main thread and then move them to the other thread
   * afterwards (which is completely legal). While annoying, this gets around the issue listed above by both preventing
   * the main thread from using the context during initialization and preventing more than one shared context being made
   * at the same time (which may or may not actually make a difference).
   */
  bool Init();

  void Close();

  void FrameToValue(FramePtr frame, StreamPtr stream, NodeValueTable* table);

  void RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params);

  void TextureToBuffer(const QVariant& texture, void *buffer);

  void SetParameters(const VideoRenderingParams& params);

private:
  QOpenGLContext* ctx_;
  QOffscreenSurface surface_;

  QOpenGLFunctions* functions_;

  OpenGLFramebuffer buffer_;

  ColorProcessorCache color_cache_;

  VideoRenderingParams video_params_;

  OpenGLShaderCache shader_cache_;

  OpenGLTextureCache texture_cache_;

  struct CachedStill {
    OpenGLTextureCache::ReferencePtr texture;
    QString colorspace;
    bool alpha_is_associated;
    int divider;
  };

  RenderCache<Stream*, CachedStill> still_image_cache_;

private slots:
  void FinishInit();

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLPROXY_H
