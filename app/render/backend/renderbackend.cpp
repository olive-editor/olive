#include "renderbackend.h"

RenderBackend::RenderBackend() :
  viewer_node_(nullptr)
{

}

RenderBackend::~RenderBackend()
{
}

const QString &RenderBackend::GetError()
{
  return error_;
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ != nullptr) {
    disconnect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(Compile()));
  }

  viewer_node_ = viewer_node;

  if (viewer_node_ != nullptr) {
    connect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(Compile()));
  }

  Decompile();
}

void RenderBackend::SetError(const QString &error)
{
  error_ = error;
}

ViewerOutput *RenderBackend::viewer_node() const
{
  return viewer_node_;
}
