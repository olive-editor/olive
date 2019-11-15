#include "audiobackend.h"

#include "audioworker.h"

AudioBackend::AudioBackend(QObject *parent) :
  AudioRenderBackend(parent)
{
}

AudioBackend::~AudioBackend()
{
  Close();
}

QIODevice *AudioBackend::GetAudioPullDevice()
{
  pull_device_.setFileName(CachePathName());

  return &pull_device_;
}

bool AudioBackend::InitInternal()
{
  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    // Create one processor object for each thread
    AudioWorker* processor = new AudioWorker(decoder_cache());
    processor->SetParameters(params());
    processors_.append(processor);
  }

  return true;
}

void AudioBackend::CloseInternal()
{
  // This backend doesn't init anything yet
}

bool AudioBackend::CompileInternal()
{
  // This backend doesn't compile anything yet
  return true;
}

void AudioBackend::DecompileInternal()
{
  // This backend doesn't compile anything yet
}

void AudioBackend::ConnectWorkerToThis(RenderWorker *worker)
{
  connect(worker, SIGNAL(CompletedCache(NodeDependency)), this, SLOT(ThreadCompletedCache(NodeDependency)));
}

void AudioBackend::ThreadCompletedCache(NodeDependency dep)
{
  caching_ = false;

  QByteArray cached_samples = dep.node()->get_cached_value(dep.range()).toByteArray();

  int offset = params().time_to_bytes(dep.in());
  int length = params().time_to_bytes(dep.range().length());
  int out_point = offset + length;

  if (pcm_data_.size() < out_point) {
    pcm_data_.resize(out_point);
  }

  // Replace data with this data
  memcpy(pcm_data_.data() + offset, cached_samples.data(), static_cast<size_t>(length));

  QFile f(CachePathName());
  if (f.open(QFile::WriteOnly)) {
    f.write(pcm_data_);
    f.close();
  }

  CacheNext();
}
