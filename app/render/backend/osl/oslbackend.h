#ifndef OSLBACKEND_H
#define OSLBACKEND_H

#include "../videorenderbackend.h"

class OSLBackend : public VideoRenderBackend
{
  Q_OBJECT
public:
  OSLBackend(QObject* parent = nullptr);

  virtual ~OSLBackend() override;

protected:
  virtual bool InitInternal() override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

  //virtual void ParamsChangedEvent() override;

};

#endif // OSLBACKEND_H
