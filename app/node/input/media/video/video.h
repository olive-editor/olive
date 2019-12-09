#ifndef VIDEOINPUT_H
#define VIDEOINPUT_H

#include <QOpenGLTexture>

#include "../media.h"
#include "render/colormanager.h"

class VideoInput : public MediaInput
{
public:
  VideoInput();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  NodeInput* matrix_input() const;

  virtual bool IsAccelerated() const override;
  virtual QString CodeVertex() const override;
  virtual QString CodeFragment() const override;

  virtual void Retranslate() override;

protected:

private:
  NodeInput* matrix_input_;

};

#endif // VIDEOINPUT_H
