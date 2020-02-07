#include "oslworker.h"

#include "common/clamp.h"
#include "core.h"
#include "node/block/transition/transition.h"
#include "node/node.h"
#include "render/colormanager.h"
#include "render/pixelservice.h"

OSLWorker::OSLWorker(VideoRenderFrameCache *frame_cache, QObject *parent) :
  VideoRenderWorker(frame_cache, parent)
{
}

void OSLWorker::FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable *table)
{
  Q_UNUSED(stream)
  Q_UNUSED(frame)
  Q_UNUSED(table)
}

void OSLWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable *output_params)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(input_params)
  Q_UNUSED(output_params)
}

void OSLWorker::TextureToBuffer(const QVariant &tex_in, QByteArray &buffer)
{
  Q_UNUSED(tex_in)
  Q_UNUSED(buffer)
}
