#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include "../videorenderbackend.h"
#include "openglframebuffer.h"
#include "openglworker.h"
#include "opengltexture.h"
#include "openglshader.h"

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
  struct CompiledNode {
    QString id;
    OpenGLShaderPtr program;
  };

  bool TraverseCompiling(Node* n);

  OpenGLShaderPtr GetShaderFromID(const QString& id);

  QString GenerateShaderID(NodeOutput* output);

  QList<CompiledNode> compiled_nodes_;

  QVector<OpenGLWorker*> processors_;

  OpenGLTexturePtr master_texture_;
  rational push_time_;

  OpenGLFramebuffer copy_buffer_;
  OpenGLShaderPtr copy_pipeline_;

private slots:
  void ThreadCallback(OpenGLTexturePtr texture, const rational& time, const QByteArray& hash);

  void ThreadRequestSibling(NodeDependency dep);

  void ThreadSkippedFrame(const rational &time, const QByteArray &hash);

  void DownloadThreadComplete(const QByteArray &hash);
};

#endif // OPENGLBACKEND_H
