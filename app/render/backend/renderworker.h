#ifndef RENDERWORKER_H
#define RENDERWORKER_H

#include <QObject>

#include "node/node.h"
#include "decodercache.h"

class RenderWorker : public QObject
{
  Q_OBJECT
public:
  RenderWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

  Q_DISABLE_COPY_MOVE(RenderWorker)

  bool Init();

  bool IsStarted();

  bool IsAvailable();

public slots:
  void Close();

  void Render(NodeDependency path);

  virtual void RenderAsSibling(NodeDependency dep) = 0;

protected:
  Node* ValidateBlock(Node* n, const rational& time);

  virtual bool InitInternal() = 0;

  virtual void CloseInternal() = 0;

  virtual void RenderInternal(const NodeDependency& path);

  StreamPtr ResolveStreamFromInput(NodeInput* input);
  DecoderPtr ResolveDecoderFromInput(NodeInput* input);

  QList<Node*> ListNodeAndAllDependencies(Node* n);

  DecoderCache* decoder_cache();

  QAtomicInt working_;

private:
  bool started_;

  DecoderCache* decoder_cache_;

};

#endif // RENDERWORKER_H
