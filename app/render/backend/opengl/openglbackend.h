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

  virtual void EmitCachedFrameReady(const QList<rational> &times, const QVariant& value, qint64 job_time) override;

private:
  OpenGLTexturePtr CopyTexture(OpenGLTexturePtr input);

  OpenGLShaderCache shader_cache_;

  OpenGLTextureCache texture_cache_;

  OpenGLTexturePtr master_texture_;

  OpenGLFramebuffer copy_buffer_;
  OpenGLShaderPtr copy_pipeline_;

};

#endif // OPENGLBACKEND_H
