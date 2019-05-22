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

double H = 0.0;

void NodeMedia::Process(const rational &time)
{
  Q_UNUSED(time)

  buffer_.buffer()->BindBuffer();

  // DEBUG CODE - we should see a solid rainbow color return from this function
  H += 1;
  double S = 1.0;
  double V = 1.0;
  double C = S * V;
  double X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
  double m = V - C;
  double Rs, Gs, Bs;

  if(H >= 0 && H < 60) {
    Rs = C;
    Gs = X;
    Bs = 0;
  }
  else if(H >= 60 && H < 120) {
    Rs = X;
    Gs = C;
    Bs = 0;
  }
  else if(H >= 120 && H < 180) {
    Rs = 0;
    Gs = C;
    Bs = X;
  }
  else if(H >= 180 && H < 240) {
    Rs = 0;
    Gs = X;
    Bs = C;
  }
  else if(H >= 240 && H < 300) {
    Rs = X;
    Gs = 0;
    Bs = C;
  }
  else {
    Rs = C;
    Gs = 0;
    Bs = X;
  }

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


