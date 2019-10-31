#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include <memory>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>

#include "../renderbackend.h"
#include "openglframebuffer.h"

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

class OpenGLBackend : public RenderBackend
{
public:
  OpenGLBackend(QOpenGLContext* share_ctx);

  virtual ~OpenGLBackend() override;

  virtual bool Init() override;

  virtual void GenerateFrame(const rational& time) override;

  virtual void Close() override;

public slots:
  virtual bool Compile() override;

  virtual void Decompile() override;

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

  QVector<QThread*> threads_;
  QVector<OpenGLProcessor*> processors_;
};

#endif // OPENGLBACKEND_H
