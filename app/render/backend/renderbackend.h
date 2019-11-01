#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include "node/output/viewer/viewer.h"

class RenderBackend : public QObject
{
  Q_OBJECT
public:
  RenderBackend(QObject* parent = nullptr);
  virtual ~RenderBackend() override;

  Q_DISABLE_COPY_MOVE(RenderBackend)

  virtual bool Init() = 0;

  virtual void Close() = 0;

  const QString& GetError() const;

  void SetViewerNode(ViewerOutput* viewer_node);

public slots:
  virtual void InvalidateCache(const rational &start_range, const rational &end_range) = 0;

  virtual bool Compile() = 0;

  virtual void Decompile() = 0;

protected:
  void SetError(const QString& error);

  virtual void ViewerNodeChangedEvent(ViewerOutput* node);

  ViewerOutput* viewer_node() const;

private:
  ViewerOutput* viewer_node_;

  QString error_;
};

#endif // RENDERBACKEND_H
