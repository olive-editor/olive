#include "openglworker.h"

#include "common/clamp.h"
#include "core.h"
#include "node/block/transition/transition.h"
#include "node/node.h"
#include "openglcolorprocessor.h"
#include "openglrenderfunctions.h"
#include "render/colormanager.h"
#include "render/pixelservice.h"

OpenGLWorker::OpenGLWorker(VideoRenderFrameCache *frame_cache, QObject *parent) :
  VideoRenderWorker(frame_cache, parent)
{
}

void OpenGLWorker::FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable *table)
{
  emit RequestFrameToValue(stream, frame, table);
}

void OpenGLWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable *output_params)
{
  emit RequestRunNodeAccelerated(node, range, input_params, output_params);
}

void OpenGLWorker::TextureToBuffer(const QVariant &tex_in, QByteArray &buffer)
{
  emit RequestTextureToBuffer(tex_in, buffer);
}
