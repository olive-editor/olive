#include "audiorenderbackend.h"

#include <QDir>
#include <QtMath>

#include "common/filefunctions.h"

AudioRenderBackend::AudioRenderBackend(QObject *parent) :
  RenderBackend(parent)
{
}

void AudioRenderBackend::SetParameters(const AudioRenderingParams &params)
{
  // Since we're changing parameters, all the existing threads are invalid and must be removed. They will start again
  // next time this Node has to process anything.
  Close();

  // Set new parameters
  params_ = params;

  // Regenerate the cache ID
  RegenerateCacheID();
}

void AudioRenderBackend::InvalidateCache(const rational &start_range, const rational &end_range)
{
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(viewer_node()->Length(), end_range);

  // Truncate to length if necessary
  int max_length_in_bytes = params().time_to_bytes(viewer_node()->Length());
  if (pcm_data_.size() > max_length_in_bytes) {
    pcm_data_.resize(max_length_in_bytes);
  }

  // Add the range to the list
  cache_queue_.append(TimeRange(start_range_adj, end_range_adj));

  // Remove any overlaps so we don't render the same thing twice
  ValidateRanges();

  // Start caching cycle if it hasn't started already
  CacheNext();
}

void AudioRenderBackend::ConnectViewer(ViewerOutput *node)
{
  connect(node, SIGNAL(AudioChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));

  // FIXME: Hardcoded format
  SetParameters(AudioRenderingParams(node->audio_params(), SAMPLE_FMT_FLT));
}

void AudioRenderBackend::DisconnectViewer(ViewerOutput *node)
{
  disconnect(node, SIGNAL(AudioChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
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

void AudioRenderBackend::ValidateRanges()
{
  for (int i=0;i<cache_queue_.size();i++) {
    const TimeRange& range1 = cache_queue_.at(i);

    for (int j=0;j<cache_queue_.size();j++) {
      const TimeRange& range2 = cache_queue_.at(j);

      if (TimeRange::Overlap(range1, range2) && i != j) {
        // Combine with the first range
        cache_queue_[i] = TimeRange::Combine(range1, range2);

        // Remove the second range
        cache_queue_.removeAt(j);
        j--;
      }
    }
  }
}

QString AudioRenderBackend::CachePathName()
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id());
  this_cache_dir.mkpath(".");

  return this_cache_dir.filePath(QStringLiteral("pcm"));
}
