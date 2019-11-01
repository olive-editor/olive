#include "renderbackend.h"

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  viewer_node_(nullptr)
{

}

RenderBackend::~RenderBackend()
{
}

const QString &RenderBackend::GetError() const
{
  return error_;
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ != nullptr) {
    disconnect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  }

  viewer_node_ = viewer_node;

  if (viewer_node_ != nullptr) {
    connect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  }

  ViewerNodeChangedEvent(viewer_node_);

  Decompile();
}

void RenderBackend::SetError(const QString &error)
{
  error_ = error;
}

void RenderBackend::ViewerNodeChangedEvent(ViewerOutput *node)
{
  Q_UNUSED(node)
}

ViewerOutput *RenderBackend::viewer_node() const
{
  return viewer_node_;
}
