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

#ifndef AUDIORENDERBACKEND_H
#define AUDIORENDERBACKEND_H

#include "common/timerange.h"
#include "renderbackend.h"

OLIVE_NAMESPACE_ENTER

class AudioRenderBackend : public RenderBackend
{
  Q_OBJECT
public:
  AudioRenderBackend(QObject* parent = nullptr);

  /**
   * @brief Set parameters of the Renderer
   *
   * The Renderer owns the buffers that are used in the rendering process and this function sets the kind of buffers
   * to use. The Renderer must be stopped when calling this function.
   */
  void SetParameters(const AudioRenderingParams &params);

  const AudioRenderingParams& params() const;

  QString CachePathName() const;

signals:
  void ParamsChanged();

  void AudioComplete();

protected:
  virtual void ConnectViewer(ViewerOutput* node) override;

  virtual void DisconnectViewer(ViewerOutput* node) override;

  /**
   * @brief Internal function for generating the cache ID
   */
  virtual bool GenerateCacheIDInternal(QCryptographicHash& hash) override;

  virtual NodeInput* GetDependentInput() override;

  virtual bool CanRender() override;

  virtual void ConnectWorkerToThis(RenderWorker* worker) override;

  virtual TimeRange PopNextFrameFromQueue() override;

  virtual void InvalidateCacheInternal(const rational &start_range, const rational &end_range, bool only_visible) override;

private:
  struct ConformWaitInfo {
    StreamPtr stream;
    AudioRenderingParams params;
    TimeRange affected_range;
    rational stream_time;

    bool operator==(const ConformWaitInfo& rhs) const;
  };

  void ListenForConformSignal(AudioStreamPtr s);

  void StopListeningForConformSignal(AudioStream *s);

  QList<ConformWaitInfo> conform_wait_info_;

  AudioRenderingParams params_;

  bool ic_from_conform_;

private slots:
  void ConformUnavailable(StreamPtr stream, TimeRange range, rational stream_time, AudioRenderingParams params);

  void ConformUpdated(OLIVE_NAMESPACE::AudioRenderingParams params);

  void TruncateCache(const rational& r);

  void FilterQueueCompleteSignal();

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIORENDERBACKEND_H
