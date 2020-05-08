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

#include "audiorenderworker.h"

#include <QDir>
#include <QFloat16>

#include "audio/audiomanager.h"
#include "audio/sumsamples.h"
#include "config/config.h"
#include "node/block/clip/clip.h"

OLIVE_NAMESPACE_ENTER

AudioRenderWorker::AudioRenderWorker(QHash<Node *, Node *> *copy_map, QObject *parent) :
  RenderWorker(parent),
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
  block_range_buffer->fill(0);

  NodeValueTable merged_table;

  // Loop through active blocks retrieving their audio
  foreach (Block* b, active_blocks) {
    TimeRange range_for_block(qMax(b->in(), range.in()),
                              qMin(b->out(), range.out()));

    int destination_offset = audio_params_.time_to_samples(range_for_block.in() - range.in());
    int max_dest_sz = audio_params_.time_to_samples(range_for_block.length());

    // Destination buffer
    NodeValueTable table = ProcessNode(NodeDependency(b, range_for_block));
    QVariant sample_val = table.Take(NodeParam::kSamples);
    SampleBufferPtr samples_from_this_block;

    if (sample_val.isNull()
        || !(samples_from_this_block = sample_val.value<SampleBufferPtr>())) {
      // If we retrieved no samples from this block, do nothing
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

    int copy_length = qMin(max_dest_sz, samples_from_this_block->sample_count_per_channel());

    // Copy samples into destination buffer
    block_range_buffer->set(samples_from_this_block->const_data(), destination_offset, copy_length);

    {
      // Save waveform to file
      Block* src_block = static_cast<Block*>(copy_map_->key(b));
      QDir local_appdata_dir(Config::Current()["DiskCachePath"].toString());
      QDir waveform_loc = local_appdata_dir.filePath(QStringLiteral("waveform"));
      waveform_loc.mkpath(".");
      QString wave_fn(waveform_loc.filePath(QString::number(reinterpret_cast<quintptr>(src_block))));
      QFile wave_file(wave_fn);

      if (wave_file.open(QFile::ReadWrite)) {
        // We use S32 as a size-compatible substitute for SampleSummer::Sum which is 4 bytes in size
        AudioRenderingParams waveform_params(SampleSummer::kSumSampleRate, audio_params_.channel_layout(), SampleFormat::SAMPLE_FMT_S32);
        int chunk_size = (audio_params().sample_rate() / waveform_params.sample_rate());

        {
          // Write metadata header
          SampleSummer::Info info;
          info.channels = audio_params_.channel_count();
          wave_file.write(reinterpret_cast<char*>(&info), sizeof(SampleSummer::Info));
        }

        qint64 start_offset = sizeof(SampleSummer::Info) + waveform_params.time_to_bytes(range_for_block.in() - b->in());
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

void AudioRenderWorker::FootageProcessingEvent(StreamPtr stream, const TimeRange &input_time, NodeValueTable *table)
{
  if (stream->type() != Stream::kAudio) {
    return;
  }

  NodeValue value = GetDataFromStream(stream, input_time);

  table->Push(value);
}

const AudioRenderingParams &AudioRenderWorker::audio_params() const
{
  return audio_params_;
}

OLIVE_NAMESPACE_EXIT
