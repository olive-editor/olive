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

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

  virtual void ConnectWorkerToThis(RenderWorker* worker) override;

private slots:
  void ThreadCompletedCache(NodeDependency dep);

private:
  QFile pull_device_;

};

#endif // AUDIOBACKEND_H
