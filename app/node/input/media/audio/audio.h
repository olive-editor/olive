#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include "../media.h"

class AudioInput : public MediaInput
{
public:
  AudioInput();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  NodeOutput* samples_output();

protected:
  virtual NodeValueTable Value(const NodeValueDatabase& value) const override;

private:
  NodeOutput* samples_output_;
};

#endif // AUDIOINPUT_H
