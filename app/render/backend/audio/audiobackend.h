#ifndef AUDIOBACKEND_H
#define AUDIOBACKEND_H

#include "../audiorenderbackend.h"

class AudioBackend : public AudioRenderBackend
{
  Q_OBJECT
public:
  AudioBackend(QObject* parent = nullptr);

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;



};

#endif // AUDIOBACKEND_H
