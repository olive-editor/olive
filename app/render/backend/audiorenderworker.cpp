#include "audiorenderworker.h"

#include "audio/audiomanager.h"

AudioRenderWorker::AudioRenderWorker(DecoderCache *decoder_cache, QObject *parent) :
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
  return decoder->RetrieveAudio(range.in(), range.out() - range.in(), audio_params_);
}

NodeValueTable AudioRenderWorker::RenderBlock(TrackOutput *track, const TimeRange &range)
{
  QList<Block*> active_blocks = track->BlocksAtTimeRange(range);

  // All these blocks will need to output to a buffer so we create one here
  QByteArray block_range_buffer(audio_params_.time_to_bytes(range.length()), 0);

  NodeValueTable merged_table;

  // Loop through active blocks retrieving their audio
  foreach (Block* b, active_blocks) {
    TimeRange range_for_block(qMax(b->in(), range.in()),
                              qMin(b->out(), range.out()));

    NodeValueTable table = RenderAsSibling(NodeDependency(b,
                                                          range_for_block));

    QByteArray samples_from_this_block = table.Take(NodeParam::kSamples).toByteArray();
    int destination_offset = audio_params_.time_to_bytes(range_for_block.in() - range.in());
    int maximum_copy_size = audio_params_.time_to_bytes(range_for_block.length());
    int copied_size = 0;

    if (!samples_from_this_block.isEmpty()) {
      copied_size = samples_from_this_block.size();

      memcpy(block_range_buffer.data()+destination_offset,
             samples_from_this_block.data(),
             static_cast<size_t>(copied_size));
    }

    if (copied_size < maximum_copy_size) {
      memset(block_range_buffer.data()+destination_offset+copied_size,
             0,
             static_cast<size_t>(maximum_copy_size - copied_size));
    }

    NodeValueTable::Merge({merged_table, table});
  }

  merged_table.Push(NodeParam::kSamples, block_range_buffer);

  return merged_table;
}
