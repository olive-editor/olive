#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include "../videorenderbackend.h"
#include "openglframebuffer.h"
#include "openglproxy.h"
#include "openglshader.h"
#include "opengltexture.h"
#include "openglworker.h"

class OpenGLBackend : public VideoRenderBackend
{
  Q_OBJECT
public:
  OpenGLBackend(QObject* parent = nullptr);

  virtual ~OpenGLBackend() override;

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

  virtual void ParamsChangedEvent() override;

private:
  OpenGLProxy* proxy_;

};

#endif // OPENGLBACKEND_H
