#ifndef AUDIOBACKEND_H
#define AUDIOBACKEND_H

#include <QFile>

#include "../audiorenderbackend.h"

class AudioBackend : public AudioRenderBackend
{
  Q_OBJECT
public:
  AudioBackend(QObject* parent = nullptr);

  virtual ~AudioBackend() override;

  virtual QIODevice* GetAudioPullDevice() override;

public slots:
  virtual void InvalidateCache(const rational &start_range, const rational &end_range) override;

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

  virtual void ConnectWorkerToThis(RenderWorker* worker) override;

private slots:
  void ThreadCompletedCache(NodeDependency dep, NodeValueTable data);

private:
  QFile pull_device_;

};

#endif // AUDIOBACKEND_H
