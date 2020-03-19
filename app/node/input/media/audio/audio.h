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

private:
};

#endif // AUDIOINPUT_H
