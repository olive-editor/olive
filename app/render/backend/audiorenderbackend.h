#ifndef AUDIORENDERBACKEND_H
#define AUDIORENDERBACKEND_H

#include "common/timerange.h"
#include "renderbackend.h"

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

  virtual QIODevice* GetAudioPullDevice() = 0;

  const AudioRenderingParams& params();

  QString CachePathName();

protected:
  virtual void ConnectViewer(ViewerOutput* node) override;

  virtual void DisconnectViewer(ViewerOutput* node) override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

  /**
   * @brief Internal function for generating the cache ID
   */
  virtual bool GenerateCacheIDInternal(QCryptographicHash& hash) override;

  virtual NodeInput* GetDependentInput() override;

  virtual bool CanRender() override;

  virtual void ConnectWorkerToThis(RenderWorker* worker) override;

  QHash<Node*, Node*> copy_map_;

private:
  struct ConformWaitInfo {
    StreamPtr stream;
    AudioRenderingParams params;
    TimeRange affected_range;
    rational stream_time;

    bool operator==(const ConformWaitInfo& rhs) const;
  };

  QList<ConformWaitInfo> conform_wait_info_;

  AudioRenderingParams params_;

private slots:
  void ConformUnavailable(StreamPtr stream, const TimeRange& range, const rational& stream_time, const AudioRenderingParams &params);

  void ConformUpdated(Stream *stream, const AudioRenderingParams& params);

};

#endif // AUDIORENDERBACKEND_H
