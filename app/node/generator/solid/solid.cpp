#include "solid.h"

SolidGenerator::SolidGenerator() :
  texture_(nullptr)
{
  color_input_ = new NodeInput();
  color_input_->add_data_input(NodeParam::kColor);
  AddParameter(color_input_);

  texture_output_ = new NodeOutput();
  texture_output_->set_data_type(NodeOutput::kTexture);
  AddParameter(texture_output_);
}

QString SolidGenerator::Name()
{
  return tr("Solid");
}

QString SolidGenerator::id()
{
  return "org.olivevideoeditor.Olive.solidgenerator";
}

QString SolidGenerator::Category()
{
  return tr("Generator");
}

QString SolidGenerator::Description()
{
  return tr("Generate a solid color.");
}

NodeOutput *SolidGenerator::texture_output()
{
  return texture_output_;
}

void SolidGenerator::Process(const rational &time)
{
  // FIXME: Test code
  Q_UNUSED(time)

  if (texture_ == nullptr) {
    QImage img(1920, 1080, QImage::Format_RGBA8888_Premultiplied);
    img.fill(Qt::red);

    texture_ = new QOpenGLTexture(img);
  }

  texture_output_->set_value(texture_->textureId());
  // End test code
}
