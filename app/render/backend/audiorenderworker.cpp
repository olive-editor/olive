#include "audiorenderworker.h"

#include <QDir>
#include <QFloat16>

#include "audio/audiomanager.h"
#include "audio/sumsamples.h"
#include "config/config.h"
#include "node/block/clip/clip.h"

AudioRenderWorker::AudioRenderWorker(DecoderCache* decoder_cache, QHash<Node *, Node *> *copy_map, QObject *parent) :
  RenderWorker(decoder_cache, parent),
  copy_map_(copy_map)
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

NodeValueTable AudioRenderWorker::RenderBlock(const TrackOutput *track, const TimeRange &range)
{
  QList<Block*> active_blocks = track->BlocksAtTimeRange(range);

  // All these blocks will need to output to a buffer so we create one here
  SampleBufferPtr block_range_buffer = SampleBuffer::CreateAllocated(audio_params_, audio_params_.time_to_samples(range.length()));

  NodeValueTable merged_table;

  // Loop through active blocks retrieving their audio
  foreach (Block* b, active_blocks) {
    TimeRange range_for_block(qMax(b->in(), range.in()),
                              qMin(b->out(), range.out()));

    // Destination buffer
    NodeValueTable table = ProcessNode(NodeDependency(b, range_for_block));
    QVariant sample_val = table.Take(NodeParam::kSamples);

    if (sample_val.isNull()) {
      continue;
    }

    SampleBufferPtr samples_from_this_block = sample_val.value<SampleBufferPtr>();

    if (!samples_from_this_block) {
      continue;
    }

    // Stretch samples here
    rational abs_speed = qAbs(b->speed());

    if (abs_speed != 1) {
      samples_from_this_block->speed(abs_speed.toDouble());
    }

    if (b->is_reversed()) {
      // Reverse the audio buffer
      samples_from_this_block->reverse();
    }

    int destination_offset = audio_params_.time_to_samples(range_for_block.in() - range.in()) * sizeof(float);
    int max_dest_sz = audio_params_.time_to_samples(range_for_block.length()) * sizeof(float);
    int actual_copy_size = qMin(max_dest_sz, static_cast<int>(samples_from_this_block->sample_count_per_channel() * sizeof(float)));

    for (int i=0;i<audio_params_.channel_count();i++) {
      char* dst_ptr = reinterpret_cast<char*>(block_range_buffer->data()[i]) + destination_offset;

      if (actual_copy_size > 0) {
        memcpy(dst_ptr,
               samples_from_this_block->data()[i],
               actual_copy_size);
      }

      if (actual_copy_size < max_dest_sz) {
        memset(dst_ptr + actual_copy_size, 0, max_dest_sz - actual_copy_size);
      }
    }

    {
      // Save waveform to file
      Block* src_block = static_cast<Block*>(copy_map_->value(b));
      QDir local_appdata_dir(Config::Current()["DiskCachePath"].toString());
      QDir waveform_loc = local_appdata_dir.filePath("waveform");
      waveform_loc.mkpath(".");
      QFile wave_file(waveform_loc.filePath(QString::number(reinterpret_cast<quintptr>(src_block))));

      if (wave_file.open(QFile::ReadWrite)) {
        // We use S32 as a size-compatible substitute for SampleSummer::Sum which is 4 bytes in size
        AudioRenderingParams waveform_params(SampleSummer::kSumSampleRate, audio_params_.channel_layout(), SampleFormat::SAMPLE_FMT_S32);
        int chunk_size = (audio_params().sample_rate() / waveform_params.sample_rate());

        qint64 start_offset = waveform_params.time_to_bytes(range_for_block.in() - b->in());
        qint64 length_offset = waveform_params.time_to_bytes(range_for_block.length());
        qint64 end_offset = start_offset + length_offset;

        if (wave_file.size() < end_offset) {
          wave_file.resize(end_offset);
        }

        wave_file.seek(start_offset);

        for (int i=0;i<samples_from_this_block->sample_count_per_channel();i+=chunk_size) {
          QVector<SampleSummer::Sum> summary = SampleSummer::SumSamples(samples_from_this_block,
                                                                        i,
                                                                        qMin(chunk_size, samples_from_this_block->sample_count_per_channel() - i));

          wave_file.write(reinterpret_cast<const char*>(summary.constData()),
                          summary.size() * sizeof(SampleSummer::Sum));
        }

        wave_file.close();

        if (src_block->type() == Block::kClip) {
          emit static_cast<ClipBlock*>(src_block)->PreviewUpdated();
        }
      }
    }

    NodeValueTable::Merge({merged_table, table});
  }

  merged_table.Push(NodeParam::kSamples, QVariant::fromValue(block_range_buffer));

  return merged_table;
}

const AudioRenderingParams &AudioRenderWorker::audio_params() const
{
  return audio_params_;
}
