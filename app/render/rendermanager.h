/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include "threading/threadpool.h"

namespace olive {

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

  enum ReturnType {
    kTexture,
    kFrame
  };

  struct RenderVideoParams {
    RenderVideoParams(Node *n, const VideoParams &vparam, const AudioParams &aparam, const rational &t,
                ColorManager *colorman)
    {
      node = n;
      video_params = vparam;
      audio_params = aparam;
      time = t;
      color_manager = colorman;
      use_cache = false;
      priority = RenderTicketPriority::kNormal;
      return_type = kFrame;
      force_format = VideoParams::kFormatInvalid;
      force_color_output = nullptr;
      force_size = QSize(0, 0);
    }

    void AddCache(FrameHashCache *cache)
    {
      cache_dir = cache->GetCacheDirectory();
      cache_timebase = cache->GetTimebase();
      cache_id = cache->GetUuid().toString();
    }

    Node *node;
    VideoParams video_params;
    AudioParams audio_params;
    rational time;
    ColorManager *color_manager;
    bool use_cache;
    RenderTicketPriority priority;
    ReturnType return_type;

    QString cache_dir;
    rational cache_timebase;
    QString cache_id;

    QSize force_size;
    QMatrix4x4 force_matrix;
    VideoParams::Format force_format;
    ColorProcessorPtr force_color_output;
  };

  /**
   * @brief Asynchronously generate a frame at a given time
   *
   * The ticket from this function will return a FramePtr - the rendered frame in reference color
   * space.
   *
   * This function is thread-safe.
   */
  RenderTicketPtr RenderFrame(const RenderVideoParams &params);

  struct RenderAudioParams {
    RenderAudioParams(Node *n, const TimeRange &time, const AudioParams &aparam)
    {
      node = n;
      range = time;
      audio_params = aparam;
      generate_waveforms = false;
      priority = RenderTicketPriority::kNormal;
      clamp = true;
    }

    Node *node;
    TimeRange range;
    AudioParams audio_params;
    bool generate_waveforms;
    RenderTicketPriority priority;
    bool clamp;
  };

  /**
   * @brief Asynchronously generate a chunk of audio
   *
   * The ticket from this function will return a SampleBufferPtr - the rendered audio.
   *
   * This function is thread-safe.
   */
  RenderTicketPtr RenderAudio(const RenderAudioParams &params);

  virtual void RunTicket(RenderTicketPtr ticket) const override;

  enum TicketType {
    kTypeVideo,
    kTypeAudio
  };

  Backend backend() const
  {
    return backend_;
  }

  static int GetNumberOfIdealConcurrentJobs()
  {
    return QThread::idealThreadCount();
  }

signals:

private:
  RenderManager(QObject* parent = nullptr);

  virtual ~RenderManager() override;

  static RenderManager* instance_;

  Renderer* context_;

  Backend backend_;

  DecoderCache* decoder_cache_;

  ShaderCache* shader_cache_;

  QVariant default_shader_;

};

}

Q_DECLARE_METATYPE(olive::RenderManager::TicketType)

#endif // RENDERBACKEND_H
