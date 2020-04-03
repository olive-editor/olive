#include "video.h"

#include <QDebug>
#include <QMatrix4x4>
#include <QOpenGLPixelTransferOptions>

#include "codec/ffmpeg/ffmpegdecoder.h"
#include "core.h"
#include "project/item/footage/footage.h"
#include "render/pixelformat.h"

VideoInput::VideoInput()
{
  matrix_input_ = new NodeInput("matrix_in", NodeInput::kMatrix);
  AddInput(matrix_input_);
}

Node *VideoInput::copy() const
{
  return new VideoInput();
}

QString VideoInput::Name() const
{
  return tr("Video Input");
}

QString VideoInput::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.videoinput");
}

QString VideoInput::Description() const
{
  return tr("Import a video footage stream.");
}

NodeInput *VideoInput::matrix_input() const
{
  return matrix_input_;
}

Node::Capabilities VideoInput::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString VideoInput::ShaderVertexCode(const NodeValueDatabase&) const
{
  return ReadFileAsString(":/shaders/videoinput.vert");
}

QString VideoInput::ShaderFragmentCode(const NodeValueDatabase&) const
{
  return ReadFileAsString(":/shaders/videoinput.frag");
}

void VideoInput::Retranslate()
{
  MediaInput::Retranslate();

  matrix_input_->set_name(tr("Transform"));
}
