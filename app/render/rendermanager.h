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

#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include <QtConcurrent/QtConcurrent>

#include "config/config.h"
#include "colorprocessorcache.h"
#include "dialog/rendercancel/rendercancel.h"
#include "node/graph.h"
#include "node/output/viewer/viewer.h"
#include "node/traverser.h"
#include "render/renderer.h"
#include "rendercache.h"
#include "stillimagecache.h"
#include "threading/threadpool.h"

OLIVE_NAMESPACE_ENTER

class RenderManager : public ThreadPool
{
  Q_OBJECT
public:
  enum Backend {
    /// Graphics acceleration provided by OpenGL
    kOpenGL,

    /// No graphics rendering - used to test core threading logic
    kDummy
  };

  static void CreateInstance()
  {
    instance_ = new RenderManager();
  }

  static void DestroyInstance()
  {
    delete instance_;
    instance_ = nullptr;
  }

  static RenderManager* instance()
  {
    return instance_;
  }

  /**
   * @brief Generate a unique identifier for a certain node at a certain time
   */
  static QByteArray Hash(const Node *n, const VideoParams &params, const rational &time);

  /**
   * @brief Asynchronously generate a frame at a given time
   *
   * The ticket from this function will return a FramePtr - the rendered frame in reference color
   * space.
   *
   * Setting `prioritize` to TRUE puts this ticket at the top of the queue. Leaving it as FALSE
   * appends it to the bottom.
   *
   * This function is thread-safe.
   */
  RenderTicketPtr RenderFrame(ViewerOutput* viewer, ColorManager* color_manager, const rational& time, RenderMode::Mode mode, FrameHashCache* cache = nullptr, bool prioritize = false);
  RenderTicketPtr RenderFrame(ViewerOutput* viewer, ColorManager* color_manager, const rational& time, RenderMode::Mode mode, const QSize& force_size, const QMatrix4x4& matrix, FrameHashCache* cache = nullptr, bool prioritize = false);

  /**
   * @brief Asynchronously generate a chunk of audio
   *
   * The ticket from this function will return a SampleBufferPtr - the rendered audio.
   *
   * Setting `prioritize` to TRUE puts this ticket at the top of the queue. Leaving it as FALSE
   * appends it to the bottom.
   *
   * This function is thread-safe.
   */
  RenderTicketPtr RenderAudio(ViewerOutput* viewer, const TimeRange& r, bool generate_waveforms, bool prioritize = false);

  RenderTicketPtr SaveFrameToCache(FrameHashCache* cache, FramePtr frame, const QByteArray& hash, bool prioritize = false);

  virtual void RunTicket(RenderTicketPtr ticket) const override;

  enum TicketType {
    kTypeVideo,
    kTypeAudio,
    kTypeVideoDownload
  };

  Backend backend() const
  {
    return backend_;
  }

signals:

private:
  RenderManager(QObject* parent = nullptr);

  virtual ~RenderManager() override;

  static RenderManager* instance_;

  Renderer* context_;

  Backend backend_;

  StillImageCache* still_cache_;

  DecoderCache* decoder_cache_;

  ShaderCache* shader_cache_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::RenderManager::TicketType);

#endif // RENDERBACKEND_H
