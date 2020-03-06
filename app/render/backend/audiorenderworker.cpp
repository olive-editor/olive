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

        // Save waveform to file
        Block* src_block = static_cast<Block*>(copy_map_->value(b));
        QDir local_appdata_dir(Config::Current()["DiskCachePath"].toString());
        QDir waveform_loc = local_appdata_dir.filePath("waveform");
        waveform_loc.mkpath(".");
        QFile wave_file(waveform_loc.filePath(QString::number(reinterpret_cast<quintptr>(src_block))));
        wave_file.open(QFile::ReadWrite);

        // FIXME: Assumes 32-bit float
        float* flt_samples = reinterpret_cast<float*>(samples_from_this_block.data());
        int nb_sample = samples_from_this_block.size() / sizeof(float);

        // FIXME: Hardcoded sample rate
        // We use S16 here as a size-compatible substitute for qfloat16/half
        AudioRenderingParams waveform_params(SampleSummer::kSumSampleRate, audio_params_.channel_layout(), SampleFormat::SAMPLE_FMT_S16);
        int chunk_size = (audio_params().sample_rate() / waveform_params.sample_rate()) * waveform_params.channel_count();

        qint64 start_offset = waveform_params.time_to_bytes(range_for_block.in() - b->in()) * 2;
        qint64 length_offset = waveform_params.time_to_bytes(range_for_block.length()) * 2;
        qint64 end_offset = start_offset + length_offset;

        if (wave_file.size() < end_offset) {
          wave_file.resize(end_offset);
        }

        wave_file.seek(start_offset);

        for (int i=0;i<nb_sample;i+=chunk_size) {
          QVector<SampleSummer::Sum> summary = SampleSummer::SumSamples(&flt_samples[i],
                                                                        qMin(chunk_size, nb_sample - i),
                                                                        audio_params().channel_count());

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

  merged_table.Push(NodeParam::kSamples, block_range_buffer);

  return merged_table;
}

const AudioRenderingParams &AudioRenderWorker::audio_params() const
{
  return audio_params_;
}
