#include "image.h"

ImageInput::ImageInput() :
  texture_(nullptr)
{
  texture_output_ = new NodeOutput();
  texture_output_->set_data_type(NodeOutput::kTexture);
  AddParameter(texture_output_);
}

QString ImageInput::Name()
{
  return tr("Image");
}

QString ImageInput::id()
{
  return "org.olivevideoeditor.Olive.imageinput";
}

QString ImageInput::Category()
{
  return tr("Input");
}

QString ImageInput::Description()
{
  return tr("Import an image file.");
}

NodeOutput *ImageInput::texture_output()
{
  return texture_output_;
}

void ImageInput::Process(const rational &time)
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
