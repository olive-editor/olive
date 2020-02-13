#ifndef OSLWORKER_H
#define OSLWORKER_H

#include <OpenImageIO/imagebufalgo.h>
#include <OSL/oslexec.h>

#include "../videorenderworker.h"
#include "oslshadercache.h"

class OSLWorker : public VideoRenderWorker
{
  Q_OBJECT
public:
  OSLWorker(VideoRenderFrameCache* frame_cache,
            OSL::ShadingSystem* shading_system,
            OSLShaderCache* shader_cache,
            ColorProcessorCache* color_cache,
            QObject* parent = nullptr);

protected:
  virtual void FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable* table) override;

  virtual void RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params) override;

  virtual void TextureToBuffer(const QVariant& texture, QByteArray& buffer) override;

private:
  QString ImageBufToTexture(const OIIO::ImageBuf& buffer, int tex_no);

  OSL::ShadingSystem* shading_system_;

  OSLShaderCache* shader_cache_;

  ColorProcessorCache* color_cache_;

};

using OIIOImageBufRef = std::shared_ptr<OIIO::ImageBuf>;
Q_DECLARE_METATYPE(OIIOImageBufRef)

#endif // OSLWORKER_H
