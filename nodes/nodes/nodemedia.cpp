#include "nodemedia.h"

#include <QDebug>

#include "nodes/nodegraph.h"

NodeMedia::NodeMedia(NodeGraph* c) :
  Node(c),
  buffer_(c->memory_cache())
{
  matrix_input_ = new NodeIO(this, "matrix", tr("Matrix"), true, false);
  matrix_input_->AddAcceptedNodeInput(olive::nodes::kMatrix);

  texture_output_ = new NodeIO(this, "texture", tr("Texture"), true, false);
  texture_output_->SetOutputDataType(olive::nodes::kTexture);
}

QString NodeMedia::name()
{
  return tr("Media");
}

QString NodeMedia::id()
{
  return "org.olivevideoeditor.Olive.media";
}

void NodeMedia::Process(rational time)
{
  Q_UNUSED(time)

  buffer_.buffer()->BindBuffer();

  // DEBUG CODE - we should see a solid red color return from this function
  glClearColor(1.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  // END DEBUG CODE

  buffer_.buffer()->ReleaseBuffer();

  qDebug() << "Node Media has" << buffer_.buffer()->texture();

  texture_output_->SetValue(buffer_.buffer()->texture());
}

NodeIO *NodeMedia::matrix_input()
{
  return matrix_input_;
}

NodeIO *NodeMedia::texture_output()
{
  return texture_output_;
}


