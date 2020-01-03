#include "audiorenderbackend.h"

#include <QDir>
#include <QtMath>

#include "audiorenderworker.h"
#include "common/filefunctions.h"

AudioRenderBackend::AudioRenderBackend(QObject *parent) :
  RenderBackend(parent)
{
}

void AudioRenderBackend::SetParameters(const AudioRenderingParams &params)
{
  // Set new parameters
  params_ = params;

  // Set params on all processors
  // FIXME: Undefined behavior if the processors are currently working, this may need to be delayed like the
  //        recompile signal
  foreach (RenderWorker* worker, processors_) {
    static_cast<AudioRenderWorker*>(worker)->SetParameters(params_);
  }

  // Regenerate the cache ID
  RegenerateCacheID();
}

void AudioRenderBackend::ConnectViewer(ViewerOutput *node)
{
  connect(node, SIGNAL(AudioChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  connect(node, SIGNAL(AudioGraphChanged()), this, SLOT(QueueRecompile()));
}

void AudioRenderBackend::DisconnectViewer(ViewerOutput *node)
{
  disconnect(node, SIGNAL(AudioChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  disconnect(node, SIGNAL(AudioGraphChanged()), this, SLOT(QueueRecompile()));
}

bool AudioRenderBackend::GenerateCacheIDInternal(QCryptographicHash &hash)
{
  if (!params_.is_valid()) {
    return false;
  }

  // Generate an ID that is more or less guaranteed to be unique to this Sequence
  hash.addData(QString::number(params_.sample_rate()).toUtf8());
  hash.addData(QString::number(params_.channel_layout()).toUtf8());
  hash.addData(QString::number(params_.format()).toUtf8());

  return true;
}

const AudioRenderingParams &AudioRenderBackend::params()
{
  return params_;
}

NodeInput *AudioRenderBackend::GetDependentInput()
{
  return viewer_node()->samples_input();
}

QString AudioRenderBackend::CachePathName()
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id());
  this_cache_dir.mkpath(".");

  return this_cache_dir.filePath(QStringLiteral("pcm"));
}

bool AudioRenderBackend::CanRender()
{
  return params_.is_valid();
}
