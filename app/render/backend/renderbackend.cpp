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

void RenderBackend::set_viewer_node(ViewerOutput *viewer_node)
{
  viewer_node_ = viewer_node;

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
