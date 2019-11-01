#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include <memory>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>

#include "../videorenderbackend.h"
#include "openglframebuffer.h"
#include "opengltexture.h"
#include "openglshader.h"

class OpenGLProcessor : public QObject {
public:
  OpenGLProcessor(QOpenGLContext* share_ctx, QObject* parent = nullptr);

  virtual ~OpenGLProcessor() override;

  Q_DISABLE_COPY_MOVE(OpenGLProcessor)

  bool IsStarted();

  void SetParameters(const VideoRenderingParams& video_params);



public slots:
  void Init();

  void Close();

signals:


private:
  void UpdateViewportFromParams();

  QOpenGLContext* share_ctx_;

  QOpenGLContext* ctx_;
  QOffscreenSurface surface_;

  QOpenGLFunctions* functions_;

  OpenGLFramebuffer buffer_;

  VideoRenderingParams video_params_;
};

class OpenGLBackend : public VideoRenderBackend
{
public:
  OpenGLBackend(QOpenGLContext* share_ctx, QObject* parent = nullptr);

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
  QOpenGLContext* share_ctx_;

  struct CompiledNode {
    QString id;
    QOpenGLShaderProgram* program;
  };

  bool TraverseCompiling(Node* n);

  QOpenGLShaderProgram *GetShaderFromID(const QString& id);

  QString GenerateShaderID(NodeOutput* output);

  QList<CompiledNode> compiled_nodes_;

  QVector<OpenGLProcessor*> processors_;

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
