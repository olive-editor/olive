#include "audiorenderbackend.h"

#include <QDir>
#include <QtMath>

#include "audiorenderworker.h"
#include "common/filefunctions.h"
#include "render/backend/indexmanager.h"

AudioRenderBackend::AudioRenderBackend(QObject *parent) :
  RenderBackend(parent)
{
  connect(IndexManager::instance(), &IndexManager::StreamConformAppended, this, &AudioRenderBackend::ConformUpdated);
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
  connect(node, &ViewerOutput::AudioChangedBetween, this, &AudioRenderBackend::InvalidateCache);
  connect(node, &ViewerOutput::AudioGraphChanged, this, &AudioRenderBackend::QueueRecompile);
}

void AudioRenderBackend::DisconnectViewer(ViewerOutput *node)
{
  disconnect(node, &ViewerOutput::AudioChangedBetween, this, &AudioRenderBackend::InvalidateCache);
  disconnect(node, &ViewerOutput::AudioGraphChanged, this, &AudioRenderBackend::QueueRecompile);
}

bool AudioRenderBackend::CompileInternal()
{
  for (int i=0;i<copied_graph_.nodes().size();i++) {
    copy_map_.insert(copied_graph_.nodes().at(i), source_node_list_.at(i));
  }

  return true;
}

void AudioRenderBackend::DecompileInternal()
{
  copy_map_.clear();
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
  QString cache_fn = cache_id();
  cache_fn.append(".pcm");
  return QDir(GetMediaCacheLocation()).filePath(cache_fn);
}

bool AudioRenderBackend::CanRender()
{
  return params_.is_valid();
}

void AudioRenderBackend::ConnectWorkerToThis(RenderWorker *worker)
{
  AudioRenderWorker* arw = static_cast<AudioRenderWorker*>(worker);

  connect(arw, &AudioRenderWorker::ConformUnavailable, this, &AudioRenderBackend::ConformUnavailable, Qt::QueuedConnection);
}

void AudioRenderBackend::ConformUnavailable(StreamPtr stream, const TimeRange &range, const rational &stream_time, const AudioRenderingParams& params)
{
  ConformWaitInfo info = {stream, params, range, stream_time};

  if (conform_wait_info_.contains(info)) {
    return;
  }

  qDebug() << "Waiting for conformed" << stream.get() << "time" << stream_time.toDouble() << "for frame" << range.in();

  AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(stream);

  if (IndexManager::instance()->IsConforming(audio_stream, params)) {

    conform_wait_info_.append(info);

  } else if (audio_stream->has_conformed_version(params)) {

    // Index JUST finished, requeue this time
    InvalidateCache(range);

  } else {

    // Start indexing process
    conform_wait_info_.append(info);
    IndexManager::instance()->StartConformingStream(audio_stream, params);

  }
}

void AudioRenderBackend::ConformUpdated(Stream *stream, const AudioRenderingParams &params)
{
  for (int i=0;i<conform_wait_info_.size();i++) {
    const ConformWaitInfo& info = conform_wait_info_.at(i);

    if (info.stream.get() == stream
        && info.params == params) {

      InvalidateCache(info.affected_range);
      conform_wait_info_.removeAt(i);
      i--;

    }
  }
}

bool AudioRenderBackend::ConformWaitInfo::operator==(const AudioRenderBackend::ConformWaitInfo &rhs) const
{
  return rhs.params == params
      && rhs.stream == stream
      && rhs.stream_time == stream_time
      && rhs.affected_range == affected_range;
}
