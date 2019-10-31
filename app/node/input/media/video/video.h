#ifndef VIDEOINPUT_H
#define VIDEOINPUT_H

#include <QOpenGLTexture>

#include "../media.h"
#include "render/colormanager.h"

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

  virtual QString Code(NodeOutput* output) override;

  //virtual void Hash(QCryptographicHash *hash, NodeOutput* from, const rational &time) override;

protected:
  //virtual QVariant Value(NodeOutput* output, const rational& in, const rational& out) override;

private:
  NodeInput* matrix_input_;

  NodeOutput* texture_output_;

};

#endif // VIDEOINPUT_H
