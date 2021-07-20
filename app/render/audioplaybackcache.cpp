/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "audioplaybackcache.h"

#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QUuid>

#include "common/filefunctions.h"

namespace olive {

const qint64 AudioPlaybackCache::kDefaultSegmentSizePerChannel = 10 * 1024 * 1024;

AudioPlaybackCache::AudioPlaybackCache(QObject* parent) :
  PlaybackCache(parent)
{
}

AudioPlaybackCache::~AudioPlaybackCache()
{
  // Segments are volatile, so delete them here
  ClearPlaylist();
}

void AudioPlaybackCache::SetParameters(const AudioParams &params)
{
  if (params_ == params) {
    return;
  }

  params_ = params;
  visual_.set_channel_count(params_.channel_count());

  // Restart empty file so there's always "something" to play
  ClearPlaylist();

  emit ParametersChanged();
}

void AudioPlaybackCache::WritePCM(const TimeRange &range, const TimeRangeList &valid_ranges, SampleBufferPtr samples, const AudioVisualWaveform *waveform)
{
  // Ensure if we have enough segments to write this data, creating more if not
  qint64 length_diff = params_.time_to_bytes_per_channel(range.out()) - playlist_.GetLength();
  while (length_diff > 0) {
    qint64 seg_sz = qMin(kDefaultSegmentSizePerChannel, length_diff);
    playlist_.push_back(CreateSegment(seg_sz, playlist_.GetLength()));
    length_diff -= seg_sz;
  }

  // Keep track of validated ranges so we can signal them all at once at the end
  TimeRangeList ranges_we_validated;

  // Calculate buffer size per channel
  qint64 buffer_size_per_channel = samples->sample_count() * params_.bytes_per_sample_per_channel();

  // Write each valid range to the segments
  foreach (const TimeRange& r, valid_ranges) {
    rational this_segment_in = 0;

    // Write PCM to playlist
    for (auto it=playlist_.begin(); it!=playlist_.end(); it++) {
      rational this_segment_out = this_segment_in + params_.bytes_per_channel_to_time((*it).size());

      if (r.in() < this_segment_out) {
        // We'll write at least something to this segment
        bool succeeded = true;

        // Calculate how much to write
        rational this_write_in_point = qMax(r.in(), this_segment_in);
        rational this_write_out_point = qMin(r.out(), this_segment_out);

        for (int i=0; i<(*it).channels(); i++) {
          QFile seg_file((*it).filename(i));

          if (seg_file.open(QFile::ReadWrite)) {
            // Calculate what the byte offsets are going to be in this segment file
            rational in_point_relative = this_write_in_point - this_segment_in;
            qint64 dst_offset = params_.time_to_bytes_per_channel(in_point_relative);

            // Calculate where to retrieve data from in the source buffer
            qint64 src_offset = params_.time_to_bytes_per_channel(this_write_in_point - range.in());

            // Determine how many bytes need to be written
            qint64 total_write_length = params_.time_to_bytes_per_channel(this_write_out_point - this_write_in_point);

            // Retrieve data buffer
            const char *a = reinterpret_cast<const char*>(samples->data(i));

            // Determine how many bytes we actually have in the source buffer
            qint64 possible_write_length = qMin(qMax(qint64(0), buffer_size_per_channel - src_offset), total_write_length);

            // Seek to our start offset
            seg_file.seek(dst_offset);

            // If we have source bytes to write, write them here
            if (possible_write_length > 0) {
              seg_file.write(a + src_offset, possible_write_length);
            }

            if (possible_write_length < total_write_length) {
              // Fill remaining space with silence
              QByteArray s(total_write_length - possible_write_length, 0x00);
              seg_file.write(s);
            }

            seg_file.close();
          } else {
            qWarning() << "Failed to write PCM data to" << seg_file.fileName();
            succeeded = false;
          }
        }

        if (succeeded) {
          ranges_we_validated.insert(TimeRange(this_write_in_point, this_write_out_point));
        }
      }

      if (r.out() <= this_segment_out) {
        // We've reached the end of this range, we can break out of the loop here
        break;
      }

      // Each segment is contiguous, so this out will be the next segment's in
      this_segment_in = this_segment_out;
    }

    // Write visual
    if (waveform) {
      visual_.OverwriteSums(*waveform, r.in(), r.in() - range.in(), r.length());
    } else {
      visual_.OverwriteSilence(r.in(), r.length());
    }
  }

  foreach (const TimeRange& v, ranges_we_validated) {
    Validate(v);
  }
}

void AudioPlaybackCache::WriteSilence(const TimeRange &range)
{
  // WritePCM will automatically fill non-existent bytes with silence, so we just have to send
  // it an empty sample buffer
  WritePCM(range, {range}, nullptr, nullptr);
}

void AudioPlaybackCache::ShiftEvent(const rational &from_in_time, const rational &to_in_time)
{
  qint64 to = params_.time_to_bytes_per_channel(to_in_time);
  qint64 from = params_.time_to_bytes_per_channel(from_in_time);

  if (from >= playlist_.GetLength()) {
    return;
  }

  int to_seg_index = playlist_.GetIndexOfPosition(to);
  int from_seg_index = playlist_.GetIndexOfPosition(from);

  qint64 from_seg_start = playlist_.at(from_seg_index).offset();
  qint64 from_seg_end = playlist_.at(from_seg_index).end();

  if (from < to) {
    // Shifting forwards, we must insert a new region and split a segment in half if necessary
    int insert_index;

    // Determine at what part of the array we'll be insert into
    if (from == from_seg_start) {
      insert_index = from_seg_index;
    } else {
      insert_index = from_seg_index + 1;

      if (from < from_seg_end) {
        // Split from segment into two
        Segment second = CloneSegment(playlist_.at(from_seg_index));

        TrimSegmentOut(&playlist_[from_seg_index], from - from_seg_start);
        TrimSegmentIn(&second, from_seg_end - from);

        playlist_.insert(insert_index, second);
      }
    }

    // Insert silent segments
    qint64 time_to_insert = to - from;

    while (time_to_insert) {
      qint64 new_seg_sz = qMin(kDefaultSegmentSizePerChannel, time_to_insert);

      // Set offset to 0 for now and fill it in later
      playlist_.insert(insert_index, CreateSegment(new_seg_sz, 0));

      time_to_insert -= new_seg_sz;
    }

    UpdateOffsetsFrom(insert_index);

  } else {
    // Shifting backwards, we'll be removing segments and truncating them if necessary
    qint64 to_seg_start = playlist_.at(to_seg_index).offset();
    qint64 to_seg_end = playlist_.at(to_seg_index).end();

    if (to_seg_index == from_seg_index) {
      if (to > to_seg_start && from < from_seg_end) {
        // Duplicate segment into two segments and trim each side to move the space in between
        Segment second = CloneSegment(playlist_.at(to_seg_index));
        from_seg_index++;
        playlist_.insert(from_seg_index, second);

        TrimSegmentOut(&playlist_[to_seg_index], to - to_seg_start);
        TrimSegmentIn(&playlist_[from_seg_index], to_seg_end - from);
      } else if (to == to_seg_start && from == from_seg_end) {
        // Segment contains entire shift area, just remove it
        RemoveSegmentFromArray(to_seg_index);
      } else if (to == to_seg_start) {
        // Trim segment in
        TrimSegmentIn(&playlist_[to_seg_index], to_seg_end - from);
      } else {
        // Assume from == to_seg_end
        TrimSegmentOut(&playlist_[to_seg_index], to_seg_end - to);
      }
    } else {
      // Remove any segments between if there are any
      while (from_seg_index != to_seg_index+1) {
        RemoveSegmentFromArray(to_seg_index+1);
        from_seg_index--;
      }

      if (from == from_seg_end) {
        RemoveSegmentFromArray(from_seg_index);
      } else {
        // Assume from > from_seg_start and trim its in point
        TrimSegmentIn(&playlist_[from_seg_index], from_seg_end - from);
      }

      if (to == to_seg_start) {
        // Assume from >= to_seg_end, remove "to" segment
        RemoveSegmentFromArray(to_seg_index);
      } else {
        TrimSegmentOut(&playlist_[to_seg_index], to_seg_end - to);
      }
    }

    UpdateOffsetsFrom(to_seg_index);
  }
}

AudioPlaybackCache::Segment AudioPlaybackCache::CloneSegment(const AudioPlaybackCache::Segment &s) const
{
  Segment new_seg = s;

  new_seg.set_channels(s.channels());

  // Copy data to a new file
  for (int i=0; i<s.channels(); i++) {
    QString new_filename = GenerateSegmentFilename();
    QFile::copy(s.filename(i), new_filename);

    new_seg.set_filename(i, new_filename);
  }

  return new_seg;
}

AudioPlaybackCache::Segment AudioPlaybackCache::CreateSegment(const qint64 &size, const qint64& offset) const
{
  Segment s(size);

  s.set_channels(params_.channel_count());

  for (int i=0; i<params_.channel_count(); i++) {
    // Generate random unused filename for this segment
    QString fn = GenerateSegmentFilename();

    // Set it for this segment/channel
    s.set_filename(i, fn);

    // Create empty file
    QFile f(fn);
    if (f.open(QFile::WriteOnly)) {
      f.close();
    }
  }

  s.set_offset(offset);

  return s;
}

QString AudioPlaybackCache::GenerateSegmentFilename() const
{
  QString new_seg_filename;

  do {
    uint32_t r = QRandomGenerator::global()->generate();
    new_seg_filename = QDir(GetCacheDirectory()).filePath(QStringLiteral("%1.pcm").arg(r));
  } while (QFileInfo::exists(new_seg_filename));

  return new_seg_filename;
}

void AudioPlaybackCache::TrimSegmentIn(AudioPlaybackCache::Segment *s, qint64 new_length)
{
  // Read filename
  for (int i=0; i<s->channels(); i++) {
    QFile f(s->filename(i));
    if (f.open(QFile::ReadWrite)) {
      // Read segment into memory, according to the size we acknowledge
      QByteArray data = f.read(s->size());

      // Trim to new length
      data = data.right(new_length);

      // Seek to start and write
      f.seek(0);

      // Write trimmed data
      f.write(data);

      f.close();
    }
  }

  s->set_size(new_length);
}

void AudioPlaybackCache::TrimSegmentOut(AudioPlaybackCache::Segment *s, qint64 new_length)
{
  // For efficiency, we don't truncate the file, we just truncate our usage of it
  s->set_size(new_length);
}

void AudioPlaybackCache::RemoveSegmentFromArray(int index)
{
  const Segment &s = playlist_.at(index);
  for (int i=0; i<s.channels(); i++) {
    QFile::remove(s.filename(i));
  }
  playlist_.removeAt(index);
}

void AudioPlaybackCache::ClearPlaylist()
{
  foreach (const Segment& s, playlist_) {
    for (int i=0; i<s.channels(); i++) {
      QFile::remove(s.filename(i));
    }
  }
  playlist_.clear();
}

void AudioPlaybackCache::UpdateOffsetsFrom(int index)
{
  qint64 current_offset;

  if (index == 0) {
    current_offset = 0;
  } else {
    const Segment& previous = playlist_.at(index - 1);

    current_offset = previous.offset() + previous.size();
  }

  for (int i=index; i<playlist_.size(); i++) {
    Segment& s = playlist_[i];

    s.set_offset(current_offset);

    current_offset += s.size();
  }
}

AudioPlaybackCache::PlaybackDevice *AudioPlaybackCache::CreatePlaybackDevice(QObject* parent) const
{
  return new PlaybackDevice(playlist_, params_.bytes_per_sample_per_channel(), parent);
}

AudioPlaybackCache::Segment::Segment(qint64 size)
{
  size_ = size;
}

AudioPlaybackCache::PlaybackDevice::PlaybackDevice(const AudioPlaybackCache::Playlist &playlist, int sample_sz, QObject *parent) :
  QIODevice(parent),
  playlist_(playlist),
  current_segment_(0),
  segment_read_index_(0),
  sample_size_(sample_sz)
{
}

AudioPlaybackCache::PlaybackDevice::~PlaybackDevice()
{
  close();
}

bool AudioPlaybackCache::PlaybackDevice::seek(qint64 pos)
{
  // Default behavior
  QIODevice::seek(pos);

  // Find which segment we're in
  current_segment_ = playlist_.GetIndexOfPosition(pos);

  // Catch failure to find index
  if (current_segment_ == -1) {
    return false;
  }

  // Find position in segment
  segment_read_index_ = pos - playlist_.at(current_segment_).offset();

  return true;
}

qint64 AudioPlaybackCache::PlaybackDevice::readData(char *data, qint64 maxSize)
{
  qint64 read_size = 0;

  while (read_size < maxSize
         && current_segment_ >= 0
         && current_segment_ < playlist_.size()) {
    const Segment& cs = playlist_.at(current_segment_);
    qint64 current_segment_sz = cs.size();

    QVector<QFile*> segment_files(cs.channels());
    segment_files.fill(nullptr);

    bool all_files_opened = true;

    // Open all file handles
    for (int i=0; i<cs.channels(); i++) {
      QFile *f = new QFile(cs.filename(i));
      segment_files[i] = f;

      if (f->open(QFile::ReadOnly)) {
        // Seek to our stored index of this segment
        f->seek(segment_read_index_);
      } else {
        all_files_opened = false;
        break;
      }
    }

    // If all file handles opened successfully, time to interleave and send them out
    if (all_files_opened) {
      // Determine how many bytes to read
      qint64 this_read_length = qMin((current_segment_sz - segment_read_index_) * cs.channels(), maxSize - read_size);

      qint64 target = read_size + this_read_length;

      while (read_size < target) {
        for (int i=0; i<cs.channels(); i++) {
          QFile *segment_file = segment_files.at(i);

          // Read those bytes
          segment_file->read(data + read_size, sample_size_);

          // Add to the read size
          read_size += sample_size_;
        }

        // Add to the read index
        segment_read_index_ += sample_size_;

        // If we've reached the end of this segment, tick the counter over to the next segment
        if (segment_read_index_ == current_segment_sz) {
          // Jump to the next file
          segment_read_index_ = 0;
          current_segment_++;
        }
      }
    }

    // Close and delete file handles
    for (int i=0; i<cs.channels(); i++) {
      QFile *f = segment_files.at(i);

      if (f) {
        if (f->isOpen()) {
          f->close();
        }
        delete f;
      }
    }
  }

  if (read_size < maxSize) {
    // Zero out remaining data
    memset(data + read_size, 0, maxSize - read_size);
  }

  //return read_size;
  return maxSize;
}

int AudioPlaybackCache::Playlist::GetIndexOfPosition(qint64 pos)
{
  if (this->isEmpty()
      || pos < 0
      || pos >= GetLength()) {
    return -1;
  }

  if (pos < this->first().size()) {
    return 0;
  }

  if (pos > this->last().offset()) {
    return this->size() - 1;
  }

  // Use a binary search to find the segment with the right offset
  int low = 0;
  int high = this->size() - 1;
  while (low <= high) {
    int mid = low + (high - low) / 2;

    const Segment& mid_segment = this->at(mid);
    if (mid_segment.offset() <= pos && mid_segment.offset() + mid_segment.size() > pos) {
      return mid;
    } else if (mid_segment.offset() < pos) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return -1;
}

qint64 AudioPlaybackCache::Playlist::GetLength() const
{
  if (this->isEmpty()) {
    return 0;
  }
  return this->last().offset() + this->last().size();
}

}
