#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include "../videorenderbackend.h"
#include "decodercache.h"
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

  virtual bool Init() override;

  virtual void Close() override;

  OpenGLTexturePtr GetCachedFrameAsTexture(const rational& time);

public slots:
  virtual bool Compile() override;

  virtual void Decompile() override;

protected:
  virtual void GenerateFrame(const rational& time) override;

private:
  bool TraverseCompiling(Node* n);

  QVector<OpenGLWorker*> processors_;

  OpenGLTexturePtr master_texture_;
  rational push_time_;

  OpenGLFramebuffer copy_buffer_;
  OpenGLShaderPtr copy_pipeline_;

  OpenGLShaderCache shader_cache_;
  DecoderCache decoder_cache_;

private slots:
  void ThreadCallback(OpenGLTexturePtr texture, const rational& time, const QByteArray& hash);

  void ThreadRequestSibling(NodeDependency dep);

  void ThreadSkippedFrame(const rational &time, const QByteArray &hash);

  void DownloadThreadComplete(const QByteArray &hash);
};

#endif // OPENGLBACKEND_H
