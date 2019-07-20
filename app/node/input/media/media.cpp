#include "media.h"

MediaInput::MediaInput() :
  texture_(nullptr)
{
  footage_input_ = new NodeInput();
  footage_input_->add_data_input(NodeInput::kFootage);
  AddParameter(footage_input_);

  texture_output_ = new NodeOutput();
  texture_output_->set_data_type(NodeOutput::kTexture);
  AddParameter(texture_output_);
}

QString MediaInput::Name()
{
  return tr("Media");
}

QString MediaInput::id()
{
  return "org.olivevideoeditor.Olive.mediainput";
}

QString MediaInput::Category()
{
  return tr("Input");
}

QString MediaInput::Description()
{
  return tr("Import a footage stream.");
}

NodeOutput *MediaInput::texture_output()
{
  return texture_output_;
}

void MediaInput::Process(const rational &time)
{
  // FIXME: Use OIIO and OCIO here

  // FIXME: Test code
  Q_UNUSED(time)

  if (texture_ == nullptr) {
    QImage img("/home/matt/Desktop/oof.png");

    texture_ = new QOpenGLTexture(img);
  }

  texture_output_->set_value(texture_->textureId());
  // End test code
}
