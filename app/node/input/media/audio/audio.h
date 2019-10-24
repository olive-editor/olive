#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include "../media.h"

class AudioInput : public MediaInput
{
public:
  AudioInput();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

protected:
  virtual QVariant Value(NodeOutput* output, const rational& in, const rational& out) override;

private:
  NodeOutput* samples_output_;
};

#endif // AUDIOINPUT_H
