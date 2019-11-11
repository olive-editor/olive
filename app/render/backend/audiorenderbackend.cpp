#include "audiorenderbackend.h"

#include <QtMath>

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
  // Add the range to the list
  cache_queue_.append(TimeRange(start_range, end_range));

  // Remove any overlaps so we don't render the same thing twice
  ValidateRanges();

  // Start caching cycle if it hasn't started already
  CacheNext();
}

void AudioRenderBackend::ViewerNodeChangedEvent(ViewerOutput *node)
{
  if (node != nullptr) {
    // FIXME: Hardcoded format
    SetParameters(AudioRenderingParams(node->audio_params(), olive::SAMPLE_FMT_FLT));
  }
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

void AudioRenderBackend::ValidateRanges()
{
  for (int i=0;i<cache_queue_.size();i++) {
    const TimeRange& range1 = cache_queue_.at(i);

    for (int j=0;j<cache_queue_.size();j++) {
      const TimeRange& range2 = cache_queue_.at(j);

      if (RangesOverlap(range1, range2) && i != j) {
        // Combine with the first range
        cache_queue_[i] = CombineRange(range1, range2);

        // Remove the second range
        cache_queue_.removeAt(j);
        j--;
      }
    }
  }
}

TimeRange AudioRenderBackend::CombineRange(const TimeRange &a, const TimeRange &b)
{
  return TimeRange(qMin(a.in(), b.in()),
                   qMax(a.out(), b.out()));
}

bool AudioRenderBackend::RangesOverlap(const TimeRange &a, const TimeRange &b)
{
  return (a.out() < b.in() && a.in() > b.out());
}
