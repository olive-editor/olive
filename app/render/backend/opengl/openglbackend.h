#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include "../videorenderbackend.h"
#include "openglframebuffer.h"
#include "openglworker.h"
#include "opengltexture.h"
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

private:
  bool TimeIsCached(const TimeRange &time);

  OpenGLTexturePtr master_texture_;

  OpenGLShaderCache shader_cache_;

private slots:
  void ThreadCompletedFrame(NodeDependency path, QByteArray hash, NodeValueTable table);
  void ThreadCompletedDownload(NodeDependency dep, QByteArray hash);
  void ThreadSkippedFrame(NodeDependency dep, QByteArray hash);
  void ThreadHashAlreadyExists(NodeDependency dep, QByteArray hash);

};

#endif // OPENGLBACKEND_H
