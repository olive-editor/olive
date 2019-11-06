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

  virtual void GenerateFrame(const rational& time) override;

  virtual bool CompileInternal() override;

  virtual void DecompileInternal() override;

private:
  bool TraverseCompiling(Node* n);

  QVector<OpenGLWorker*> processors_;

  OpenGLTexturePtr master_texture_;
  rational push_time_;

  OpenGLFramebuffer copy_buffer_;
  OpenGLShaderPtr copy_pipeline_;

  OpenGLShaderCache shader_cache_;

  bool compiled_;

private slots:
  void ThreadCompletedFrame(NodeDependency path);
  void ThreadRequestedSibling(NodeDependency dep);



  //void ThreadCallback(OpenGLTexturePtr texture, const rational& time, const QByteArray& hash);


  //void ThreadSkippedFrame(const rational &time, const QByteArray &hash);

  //void DownloadThreadComplete(const QByteArray &hash);
};

#endif // OPENGLBACKEND_H
