#ifndef VIDEOINPUT_H
#define VIDEOINPUT_H

#include <QOpenGLTexture>

#include "../media.h"
#include "render/colormanager.h"
#include "render/rendertexture.h"
#include "render/gl/shadergenerators.h"

class VideoInput : public MediaInput
{
public:
  VideoInput();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  virtual void Release() override;

  NodeInput* matrix_input();

  NodeOutput* texture_output();

  virtual void Hash(QCryptographicHash *hash, NodeOutput* from, const rational &time) override;

protected:
  virtual QVariant Value(NodeOutput* output, const rational& in, const rational& out) override;

private:
  NodeInput* matrix_input_;

  NodeOutput* texture_output_;

  RenderTexture internal_tex_;

  ColorProcessorPtr color_processor_;

  ShaderPtr pipeline_;

  QOpenGLContext* ocio_ctx_;
  GLuint ocio_texture_;

};

#endif // VIDEOINPUT_H
