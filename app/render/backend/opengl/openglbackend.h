#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include "../videorenderbackend.h"
#include "openglframebuffer.h"
#include "openglworker.h"
#include "opengltexture.h"
#include "opengltexturecache.h"
#include "openglshader.h"
#include "openglshadercache.h"

class OpenGLBackend : public VideoRenderBackend
{
  Q_OBJECT
public:
  OpenGLBackend(QObject* parent = nullptr);

  virtual ~OpenGLBackend() override;

  OpenGLTexturePtr GetCachedFrameAsTexture(const rational& time);

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

  virtual void EmitCachedFrameReady(const rational& time, const QVariant& value) override;

private:
  OpenGLTexturePtr master_texture_;

  OpenGLShaderCache shader_cache_;

  OpenGLTextureCache texture_cache_;

};

#endif // OPENGLBACKEND_H
