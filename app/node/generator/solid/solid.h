#ifndef SOLIDGENERATOR_H
#define SOLIDGENERATOR_H

#include <QOpenGLTexture>

#include "node/node.h"

class SolidGenerator : public Node
{
  Q_OBJECT
public:
  SolidGenerator();

  virtual QString Name() override;
  virtual QString Category() override;
  virtual QString Description() override;

  NodeOutput* texture_output();

public slots:
  virtual void Process(const rational &time) override;

private:
  NodeInput* color_input_;

  NodeOutput* texture_output_;

  QOpenGLTexture* texture_;
};

#endif // SOLIDGENERATOR_H
