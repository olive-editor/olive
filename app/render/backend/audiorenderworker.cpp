#include "audiorenderworker.h"

#include "audio/audiomanager.h"

AudioRenderWorker::AudioRenderWorker(DecoderCache* decoder_cache, QObject *parent) :
  RenderWorker(decoder_cache, parent)
{
}

void AudioRenderWorker::SetParameters(const AudioRenderingParams &audio_params)
{
  audio_params_ = audio_params;
}

bool AudioRenderWorker::InitInternal()
{
  // Nothing to init yet
  return true;
}

void AudioRenderWorker::CloseInternal()
{
  // Nothing to init yet
}

FramePtr AudioRenderWorker::RetrieveFromDecoder(DecoderPtr decoder, const TimeRange &range)
{
  if (decoder->HasConformedVersion(audio_params_)) {
    return decoder->RetrieveAudio(range.in(), range.out() - range.in(), audio_params_);
  } else {
    emit ConformUnavailable(decoder->stream(), CurrentPath().range(), range.out(), audio_params_);
    return nullptr;
  }
}

NodeValueTable AudioRenderWorker::RenderBlock(const TrackOutput *track, const TimeRange &range)
{
  QList<Block*> active_blocks = track->BlocksAtTimeRange(range);

  // All these blocks will need to output to a buffer so we create one here
  QByteArray block_range_buffer(audio_params_.time_to_bytes(range.length()), 0);

  NodeValueTable merged_table;

  // Loop through active blocks retrieving their audio
  foreach (Block* b, active_blocks) {
    TimeRange range_for_block(qMax(b->in(), range.in()),
                              qMin(b->out(), range.out()));

    // Destination buffer
    NodeValueTable table = ProcessNode(NodeDependency(b, range_for_block));
    QByteArray samples_from_this_block = table.Take(NodeParam::kSamples).toByteArray();

    if (!samples_from_this_block.isEmpty()) {
      // Stretch samples here
      rational abs_speed = qAbs(b->speed());

      if (abs_speed != 1) {
        QByteArray speed_adjusted_samples;

        double clip_speed = abs_speed.toDouble();

        int sample_count = audio_params_.bytes_to_samples(samples_from_this_block.size());

        for (double i=0;i<sample_count;i+=clip_speed) {
          int sample_index = qFloor(i);
          int byte_index = audio_params_.samples_to_bytes(sample_index);

          QByteArray sample_at_this_index = samples_from_this_block.mid(byte_index, audio_params_.samples_to_bytes(1));
          speed_adjusted_samples.append(sample_at_this_index);
        }

        samples_from_this_block = speed_adjusted_samples;
      }

      if (b->is_reversed()) {
        // Reverse the audio buffer
        AudioManager::ReverseBuffer(samples_from_this_block.data(), samples_from_this_block.size(), audio_params_.samples_to_bytes(1));
      }

      int destination_offset = audio_params_.time_to_bytes(range_for_block.in() - range.in());
      int actual_copy_size = qMin(audio_params_.time_to_bytes(range_for_block.length()), samples_from_this_block.size());

      if (destination_offset < 0
          || destination_offset + actual_copy_size > block_range_buffer.size()) {
        qCritical() << "Tried to copy audio beyond the size of the destination buffer";
        qCritical() << "  Destination size:" << block_range_buffer.size();
        qCritical() << "  Destination offset:" << destination_offset;
        qCritical() << "  Copy size:" << actual_copy_size;
        abort();
      } else {
        memcpy(block_range_buffer.data()+destination_offset,
               samples_from_this_block.data(),
               actual_copy_size);
      }
    }

    NodeValueTable::Merge({merged_table, table});
  }

  merged_table.Push(NodeParam::kSamples, block_range_buffer);

  return merged_table;
}

const AudioRenderingParams &AudioRenderWorker::audio_params() const
{
  return audio_params_;
}
