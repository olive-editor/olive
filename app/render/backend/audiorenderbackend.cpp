/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "audiorenderbackend.h"

#include <QDir>
#include <QtMath>

#include "audiorenderworker.h"
#include "common/filefunctions.h"
#include "render/backend/indexmanager.h"

OLIVE_NAMESPACE_ENTER

AudioRenderBackend::AudioRenderBackend(QObject *parent) :
  RenderBackend(parent),
  ic_from_conform_(false)
{
  connect(IndexManager::instance(), &IndexManager::StreamConformAppended, this, &AudioRenderBackend::ConformUpdated);
  connect(this, &AudioRenderBackend::QueueComplete, this, &AudioRenderBackend::FilterQueueCompleteSignal);
}

void AudioRenderBackend::SetParameters(const AudioRenderingParams &params)
{
  CancelQueue();

  // Set new parameters
  params_ = params;

  // Set params on all processors
  foreach (RenderWorker* worker, processors_) {
    static_cast<AudioRenderWorker*>(worker)->SetParameters(params_);
  }

  // Regenerate the cache ID
  RegenerateCacheID();

  emit ParamsChanged();
}

void AudioRenderBackend::ConnectViewer(ViewerOutput *node)
{
  connect(node, &ViewerOutput::AudioChangedBetween, this, &AudioRenderBackend::InvalidateCache);
  connect(node, &ViewerOutput::AudioGraphChanged, this, &AudioRenderBackend::QueueRecompile);
  connect(node, &ViewerOutput::LengthChanged, this, &AudioRenderBackend::TruncateCache);
}

void AudioRenderBackend::DisconnectViewer(ViewerOutput *node)
{
  disconnect(node, &ViewerOutput::AudioChangedBetween, this, &AudioRenderBackend::InvalidateCache);
  disconnect(node, &ViewerOutput::AudioGraphChanged, this, &AudioRenderBackend::QueueRecompile);
  disconnect(node, &ViewerOutput::LengthChanged, this, &AudioRenderBackend::TruncateCache);
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

TimeRange AudioRenderBackend::PopNextFrameFromQueue()
{
  TimeRange range = cache_queue_.first();

  // Limit range per worker to 2 seconds (FIXME: arbitrary, should be tweaked, maybe even in config?)
  range.set_out(qMin(range.out(), range.in() + rational(2)));

  cache_queue_.RemoveTimeRange(range);

  return range;
}

void AudioRenderBackend::InvalidateCacheInternal(const rational &start_range, const rational &end_range)
{
  if (!ic_from_conform_) {
    // Cancel any ranges waiting on a conform here since obviously the contents have changed
    TimeRange range(start_range, end_range);

    for (int i=0;i<conform_wait_info_.size();i++) {
      ConformWaitInfo& info = conform_wait_info_[i];

      // FIXME: Code copied from TimeRangeList::RemoveTimeRange()

      if (range.Contains(info.affected_range)) {
        conform_wait_info_.removeAt(i);
        i--;
      } else if (info.affected_range.Contains(range, false, false)) {
        ConformWaitInfo copy = info;

        info.affected_range.set_out(start_range);
        copy.affected_range.set_in(end_range);

        conform_wait_info_.append(copy);
      } else if (info.affected_range.in() < start_range && info.affected_range.out() > start_range) {
        info.affected_range.set_out(start_range);
      } else if (info.affected_range.in() < end_range && info.affected_range.out() > end_range) {
        info.affected_range.set_in(end_range);
      }
    }
  }

  RenderBackend::InvalidateCacheInternal(start_range, end_range);
}

void AudioRenderBackend::ConformUnavailable(StreamPtr stream, const TimeRange &range, const rational &stream_time, const AudioRenderingParams& params)
{
  ConformWaitInfo info = {stream, params, range, stream_time};

  if (conform_wait_info_.contains(info)) {
    return;
  }

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

      conform_wait_info_.removeAt(i);
      i--;

      ic_from_conform_ = true;
      InvalidateCache(info.affected_range);
      ic_from_conform_ = false;

    }
  }
}

void AudioRenderBackend::TruncateCache(const rational &r)
{
  int seq_length = params_.time_to_bytes(r);

  QFile cache_pcm(CachePathName());

  if (cache_pcm.size() > seq_length) {
    cache_pcm.resize(seq_length);
  }
}

void AudioRenderBackend::FilterQueueCompleteSignal()
{
  if (conform_wait_info_.isEmpty()) {
    emit AudioComplete();
  }
}

bool AudioRenderBackend::ConformWaitInfo::operator==(const AudioRenderBackend::ConformWaitInfo &rhs) const
{
  return rhs.params == params
      && rhs.stream == stream
      && rhs.stream_time == stream_time
      && rhs.affected_range == affected_range;
}

OLIVE_NAMESPACE_EXIT
