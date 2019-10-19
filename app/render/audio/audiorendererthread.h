#ifndef AUDIORENDERERTHREAD_H
#define AUDIORENDERERTHREAD_H

#include "audiorendererthreadbase.h"

class AudioRendererThread : public AudioRendererThreadBase
{
  Q_OBJECT
public:
  AudioRendererThread(const int& sample_rate,
                      const uint64_t& channel_layout,
                      const olive::SampleFormat& format);

public slots:
  virtual void Cancel() override;

protected:
  virtual void ProcessLoop() override;

private:
  bool cancelled_;
};

using AudioRendererThreadPtr = std::shared_ptr<AudioRendererThread>;

#endif // AUDIORENDERERTHREAD_H
