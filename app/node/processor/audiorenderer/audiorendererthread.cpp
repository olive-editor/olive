#include "audiorendererthread.h"

AudioRendererThread::AudioRendererThread(const int &sample_rate, const uint64_t &channel_layout, const olive::SampleFormat &format) :
  AudioRendererThreadBase(sample_rate, channel_layout, format),
  cancelled_(false)
{
}

void AudioRendererThread::Cancel()
{
  cancelled_ = true;

  mutex_.lock();
  wait_cond_.wakeAll();
  mutex_.unlock();

  wait();
}

void AudioRendererThread::ProcessLoop()
{

}
