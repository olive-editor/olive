#include "video.h"

#include <QDebug>
#include <QOpenGLPixelTransferOptions>

#include "core.h"
#include "decoder/ffmpeg/ffmpegdecoder.h"
#include "project/item/footage/footage.h"
#include "render/pixelservice.h"

VideoInput::VideoInput()
{
  matrix_input_ = new NodeInput("matrix_in");
  matrix_input_->set_data_type(NodeInput::kMatrix);
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
  return "org.olivevideoeditor.Olive.videoinput";
}

QString VideoInput::Category() const
{
  return tr("Input");
}

QString VideoInput::Description() const
{
  return tr("Import a video footage stream.");
}

void VideoInput::Release()
{
  MediaInput::Release();
}

NodeInput *VideoInput::matrix_input() const
{
  return matrix_input_;
}

QString VideoInput::Code() const
{
  return "#version 110\n"
         "\n"
         "varying vec2 v_texcoord;\n"
         "\n"
         "uniform sampler2D footage_in;\n"
         "uniform mat4 matrix_in;\n"
         "\n"
         "void main(void) {\n"
         "  gl_FragColor = texture2D(footage_in, vec2(vec4(v_texcoord, 0.0, 1.0) * matrix_in));\n"
         "}\n";
}

void VideoInput::Retranslate()
{
  MediaInput::Retranslate();

  matrix_input_->set_name(tr("Transform"));
}
