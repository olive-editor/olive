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

void AudioBackend::InvalidateCache(const rational &start_range, const rational &end_range)
{
  /*
  // Truncate to length if necessary
  int max_length_in_bytes = params().time_to_bytes(viewer_node()->Length());
  if (pcm_data_.size() > max_length_in_bytes) {
    pcm_data_.resize(max_length_in_bytes);
  }
  */

  AudioRenderBackend::InvalidateCache(start_range, end_range);
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

  QFile f(CachePathName());
  if (f.open(QFile::WriteOnly | QFile::Append)) {

    if (f.size() < out_point) {
      f.resize(out_point);
    }

    f.seek(offset);

    // Replace data with this data
    int copy_length = qMin(length, cached_samples.size());

    f.write(cached_samples.data(), copy_length);

    if (copy_length < length) {
      // Fill in remainder with silence
      QByteArray empty_space(length - copy_length, 0);
      f.write(empty_space);
    }

    f.close();
  }

  CacheNext();
}
