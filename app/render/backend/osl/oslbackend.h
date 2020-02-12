#ifndef OSLBACKEND_H
#define OSLBACKEND_H

#include <OSL/oslexec.h>

#include "../videorenderbackend.h"
#include "oslrenderer.h"
#include "oslshadercache.h"

class OSLBackend : public VideoRenderBackend
{
  Q_OBJECT
public:
  OSLBackend(QObject* parent = nullptr);

  virtual ~OSLBackend() override;

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

  //virtual void ParamsChangedEvent() override;

private:
  OSL::ShadingSystem* shading_system_;

  OSL::SimpleRenderer* renderer_;

  OSLShaderCache shader_cache_;

  ColorProcessorCache color_cache_;

};

#endif // OSLBACKEND_H
