#ifndef RENDERWORKER_H
#define RENDERWORKER_H

#include <QObject>

#include "node/block/block.h"
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

  virtual QVariant RenderAsSibling(NodeDependency dep) = 0;

signals:
  void RequestSibling(NodeDependency path);

  void CompletedCache(NodeDependency dep);

protected:
  /**
   * @brief Returns the block in a sequence that is active at a given time
   *
   * Blocks are connected to each other previous/next to create a BlockList or a sequence of blocks. The block that
   * is currently "active" depends on the time and only one block in a track can be active at any given time.
   *
   * Calling this function with a block and a time will traverse the provided block's track to find the block that will
   * be active at that time. The block must be valid (non-null) and the time must be valid (>= 0).
   *
   * This function may return the same block that it was called with. It will never return nullptr.
   */
  Block *ValidateBlock(Block* block, const rational& time);

  /**
   * @brief Returns all the blocks that could be active within a range of time
   *
   * Similar to ValidateBlock() but rather than returning one block for a single time, this function returns a list of
   * blocks that could be active within a range of time.
   *
   * The block must be valid (non-null) and the time must be valid (>= 0).
   *
   * The list will always contain at least one entry.
   */
  QList<Block*> ValidateBlockRange(Block* n, const TimeRange& range);

  virtual bool InitInternal() = 0;

  virtual void CloseInternal() = 0;

  virtual void RenderInternal(const NodeDependency& path);

  virtual bool OutputIsAccelerated(NodeOutput *output) = 0;

  virtual QVariant RunNodeAccelerated(NodeOutput *output) = 0;

  StreamPtr ResolveStreamFromInput(NodeInput* input);
  DecoderPtr ResolveDecoderFromInput(NodeInput* input);

  QList<Node*> ListNodeAndAllDependencies(Node* n);

  QList<NodeInput*> ProcessNodeInputsForTime(Node* n, const TimeRange& time);

  virtual QVariant FrameToValue(FramePtr frame) = 0;

  QVariant ProcessNodeNormally(const NodeDependency &dep);

  DecoderCache* decoder_cache();

  QAtomicInt working_;

private:
  bool started_;

  DecoderCache* decoder_cache_;

};

#endif // RENDERWORKER_H
