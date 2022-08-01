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
#include "render/renderticket.h"
#include "rendercache.h"

namespace olive {

class RenderThread : public QThread
{
  Q_OBJECT
public:
  RenderThread(Renderer *renderer, DecoderCache *decoder_cache, ShaderCache *shader_cache, QObject *parent = nullptr);

  void AddTicket(RenderTicketPtr ticket);

  bool RemoveTicket(RenderTicketPtr ticket);

  void quit();

protected:
  virtual void run() override;

private:
  QMutex mutex_;

  QWaitCondition wait_;

  std::list<RenderTicketPtr> queue_;

  bool cancelled_;

  Renderer *context_;

  DecoderCache *decoder_cache_;

  ShaderCache *shader_cache_;

};

class RenderManager : public QObject
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
    kFrame,
    kNull
  };

  static const rational kDryRunInterval;

  /**
   * @brief Asynchronously generate a frame at a given time
   *
   * The ticket from this function will return a FramePtr - the rendered frame in reference color
   * space.
   *
   * This function is thread-safe.
   */
  RenderTicketPtr RenderFrame(Node *node, const VideoParams &vparam, const AudioParams &param, ColorManager* color_manager,
                              const rational& time, RenderMode::Mode mode,
                              FrameHashCache* cache = nullptr, ReturnType return_type = kFrame);
  RenderTicketPtr RenderFrame(Node *node, ColorManager* color_manager,
                              const rational& time, RenderMode::Mode mode,
                              const VideoParams& video_params, const AudioParams& audio_params,
                              const QSize& force_size,
                              const QMatrix4x4& force_matrix, VideoParams::Format force_format,
                              int force_channel_count,
                              ColorProcessorPtr force_color_output,
                              FrameHashCache* cache = nullptr, ReturnType return_type = kFrame);

  /**
   * @brief Asynchronously generate a chunk of audio
   *
   * The ticket from this function will return a SampleBufferPtr - the rendered audio.
   *
   * This function is thread-safe.
   */
  RenderTicketPtr RenderAudio(Node *viewer, const TimeRange& r, const AudioParams& params, RenderMode::Mode mode, bool generate_waveforms);

  bool RemoveTicket(RenderTicketPtr ticket);

  enum TicketType {
    kTypeVideo,
    kTypeAudio
  };

  Backend backend() const
  {
    return backend_;
  }

public slots:
  void SetAggressiveGarbageCollection(bool enabled);

signals:

private:
  RenderManager(QObject* parent = nullptr);

  virtual ~RenderManager() override;

  static RenderManager* instance_;

  Renderer* context_;

  Backend backend_;

  DecoderCache* decoder_cache_;

  ShaderCache* shader_cache_;

  static constexpr auto kDecoderMaximumInactivityAggressive = 1000;
  static constexpr auto kDecoderMaximumInactivity = 5000;

  int aggressive_gc_;

  QTimer *decoder_clear_timer_;

  RenderThread *video_thread_;
  RenderThread *dry_run_thread_;
  RenderThread *audio_thread_;

private slots:
  void ClearOldDecoders();

};

}

Q_DECLARE_METATYPE(olive::RenderManager::TicketType)

#endif // RENDERBACKEND_H
