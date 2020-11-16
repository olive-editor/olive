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

#include "audioplaybackcache.h"

#include <QDir>
#include <QFile>
#include <QUuid>

#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

const qint64 AudioPlaybackCache::kDefaultSegmentSize = 5242880;

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

  // Restart empty file so there's always "something" to play
  ClearPlaylist();

  // Our current audio cache is unusable, so we truncate it automatically
  InvalidateAll();

  emit ParametersChanged();
}

void AudioPlaybackCache::WritePCM(const TimeRange &range, SampleBufferPtr samples, const qint64 &job_time)
{
  QList<TimeRange> valid_ranges = GetValidRanges(range, job_time);
  if (valid_ranges.isEmpty()) {
    return;
  }

  // Determine if we have enough segments to pull this off
  qint64 length_diff = params_.time_to_bytes(range.out()) - playlist_.GetLength();
  while (length_diff > 0) {
    qint64 seg_sz = qMin(kDefaultSegmentSize, length_diff);
    playlist_.push_back(CreateSegment(seg_sz, playlist_.GetLength()));
    length_diff -= seg_sz;
  }

  QByteArray a;
  if (samples) {
    // Convert to packed data, which is what we store on disk
    a = samples->toPackedData();
  }

  // Keep track of validated ranges so we can signal them all at once at the end
  TimeRangeList ranges_we_validated;

  // Write each valid range to the segments
  foreach (const TimeRange& r, valid_ranges) {
    rational this_segment_in = 0;

    for (auto it=playlist_.begin(); it!=playlist_.end(); it++) {
      rational this_segment_out = this_segment_in + params_.bytes_to_time((*it).size());

      if (r.in() < this_segment_out) {
        // We'll write at least something to this segment
        QFile seg_file((*it).filename());

        if (seg_file.open(QFile::ReadWrite)) {
          // Calculate how much to write
          rational this_write_in_point = qMax(r.in(), this_segment_in);
          rational this_write_out_point = qMin(r.out(), this_segment_out);

          // Calculate what the byte offsets are going to be in this segment file
          rational in_point_relative = this_write_in_point - this_segment_in;
          qint64 dst_offset = params_.time_to_bytes(in_point_relative);

          // Calculate where to retrieve data from in the source buffer
          qint64 src_offset = params_.time_to_bytes(this_write_in_point - range.in());

          // Determine how many bytes need to be written
          qint64 total_write_length = params_.time_to_bytes(this_write_out_point - this_write_in_point);

          // Determine how many bytes we actually have in the source buffer
          qint64 possible_write_length = qMin(qMax(qint64(0), a.size() - src_offset), total_write_length);

          // Seek to our start offset
          seg_file.seek(dst_offset);

          // If we have source bytes to write, write them here
          if (possible_write_length > 0) {
            seg_file.write(a.data() + src_offset, possible_write_length);
          }

          if (possible_write_length < total_write_length) {
            // Fill remaining space with silence
            QByteArray s(total_write_length - possible_write_length, 0x00);
            seg_file.write(s);
          }

          seg_file.close();

          ranges_we_validated.insert(TimeRange(this_write_in_point, this_write_out_point));
        } else {
          qWarning() << "Failed to write PCM data to" << seg_file.fileName();
        }
      }

      if (r.out() <= this_segment_out) {
        // We've reached the end of this range, we can break out of the loop here
        break;
      }

      // Each segment is contiguous, so this out will be the next segment's in
      this_segment_in = this_segment_out;
    }
  }

  foreach (const TimeRange& v, ranges_we_validated) {
    Validate(v);
  }
}

void AudioPlaybackCache::WriteSilence(const TimeRange &range, qint64 job_time)
{
  // WritePCM will automatically fill non-existent bytes with silence, so we just have to send
  // it an empty sample buffer
  WritePCM(range, nullptr, job_time);
}

void AudioPlaybackCache::ShiftEvent(const rational &from_in_time, const rational &to_in_time)
{
  if (from_in_time == to_in_time || GetLength().isNull()) {
    // Nothing to be done
    return;
  }

  qint64 to = params_.time_to_bytes(to_in_time);
  qint64 from = params_.time_to_bytes(from_in_time);

  int to_index = playlist_.GetIndexOfPosition(to);
  int from_index = playlist_.GetIndexOfPosition(from);

  qint64 from_start = playlist_.at(from_index).offset();
  qint64 from_end = playlist_.at(from_index).end();

  if (from < to) {
    // Shifting forwards, we must insert a new region and split a segment in half if necessary
    int insert_index;

    // Determine at what part of the array we'll be insert into
    if (from == from_start) {
      insert_index = from_index;
    } else {
      insert_index = from_index + 1;

      if (from < from_end) {
        // Split from segment into two
        Segment second = CloneSegment(playlist_.at(from_index));

        TrimSegmentOut(&playlist_[from_index], from - from_start);
        TrimSegmentIn(&second, from_end - from);

        playlist_.insert(insert_index, second);
      }
    }

    // Insert silent segments
    qint64 time_to_insert = to - from;

    while (time_to_insert) {
      qint64 new_seg_sz = qMin(kDefaultSegmentSize, time_to_insert);

      // Set offset to 0 for now and fill it in later
      playlist_.insert(insert_index, CreateSegment(new_seg_sz, 0));

      time_to_insert -= new_seg_sz;
    }

    UpdateOffsetsFrom(insert_index);

  } else {
    // Shifting backwards, we'll be removing segments and truncating them if necessary
    qint64 to_start = playlist_.at(to_index).offset();
    qint64 to_end = playlist_.at(to_index).end();

    if (from_index == to_index) {
      // Shift occurs in the same segment
      if (to > to_start && from < to_end) {
        // Split into two and process as normal
        Segment second = CloneSegment(playlist_.at(to_index));
        from_index++;
        playlist_.insert(from_index, second);
      } else if (to == to_start && from == to_end) {
        RemoveSegmentFromArray(to_index);
      } else if (to == to_start) {
        TrimSegmentIn(&playlist_[to_index], to_end - from);
      } else {
        TrimSegmentOut(&playlist_[from_index], to - to_start);
      }
    } else {
      // Remove all central segments (if there are any)
      while (from_index > to_index + 1) {
        RemoveSegmentFromArray(to_index + 1);
        from_index--;
      }
    }

    if (from_index != to_index) {
      // Remove or trim "to" segment
      if (to == to_start) {
        RemoveSegmentFromArray(to_index);
        from_index--;
      } else if (to < to_end) {
        TrimSegmentOut(&playlist_[to_index], to - to_start);
      }

      // Remove or trim "from" segment
      if (from == from_end) {
        RemoveSegmentFromArray(from_index);
      } else if (from > from_start) {
        TrimSegmentIn(&playlist_[from_index], from_end - from);
      }
    }

    UpdateOffsetsFrom(to_index);
  }
}

void AudioPlaybackCache::LengthChangedEvent(const rational& old, const rational& newlen)
{
  Q_UNUSED(old)

  if (!params_.is_valid()) {
    return;
  }

  qint64 new_len_in_bytes = params_.time_to_bytes(newlen);

  while (new_len_in_bytes < playlist_.GetLength()) {
    Segment& last_seg = playlist_.back();

    if (playlist_.GetLength() - last_seg.size() < new_len_in_bytes) {
      // Truncate this segment rather than removing it
      qint64 diff = playlist_.GetLength() - new_len_in_bytes;

      TrimSegmentOut(&last_seg, last_seg.size() - diff);
    } else {
      // Remove last segment
      RemoveSegmentFromArray(playlist_.size() - 1);
    }
  }
}

AudioPlaybackCache::Segment AudioPlaybackCache::CloneSegment(const AudioPlaybackCache::Segment &s) const
{
  Segment new_seg = s;

  // Copy data to a new file
  QString new_filename = GenerateSegmentFilename();
  QFile::copy(s.filename(), new_filename);

  new_seg.set_filename(new_filename);

  return new_seg;
}

AudioPlaybackCache::Segment AudioPlaybackCache::CreateSegment(const qint64 &size, const qint64& offset) const
{
  Segment s(size, GenerateSegmentFilename());

  s.set_offset(offset);

  // Create empty file
  QFile f(s.filename());
  if (f.open(QFile::WriteOnly)) {
    f.close();
  }

  return s;
}

QString AudioPlaybackCache::GenerateSegmentFilename() const
{
  QString new_seg_filename;

  do {
    uint32_t r = std::rand();
    new_seg_filename = QDir(GetCacheDirectory()).filePath(QStringLiteral("%1.pcm").arg(r));
  } while (QFileInfo::exists(new_seg_filename));

  return new_seg_filename;
}

void AudioPlaybackCache::TrimSegmentIn(AudioPlaybackCache::Segment *s, qint64 new_length)
{
  // Read filename
  QFile f(s->filename());
  if (f.open(QFile::ReadWrite)) {
    // Read whole segment into memory
    QByteArray data = f.readAll();

    // Trim to new length
    data = data.right(new_length);

    // Seek to start and write
    f.seek(0);

    // Write trimmed data
    f.write(data);

    f.close();
  }

  s->set_size(new_length);
}

void AudioPlaybackCache::TrimSegmentOut(AudioPlaybackCache::Segment *s, qint64 new_length)
{
  s->set_size(new_length);
}

void AudioPlaybackCache::RemoveSegmentFromArray(int index)
{
  QFile::remove(playlist_.at(index).filename());
  playlist_.removeAt(index);
}

void AudioPlaybackCache::ClearPlaylist()
{
  foreach (const Segment& s, playlist_) {
    QFile::remove(s.filename());
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

QList<TimeRange> AudioPlaybackCache::GetValidRanges(const TimeRange& range, const qint64& job_time)
{
  QList<TimeRange> valid_ranges;

  for (int i=jobs_.size()-1;i>=0;i--) {
    const JobIdentifier& job = jobs_.at(i);

    if (job_time >= job.job_time && job.range.OverlapsWith(range)) {
      valid_ranges.append(job.range.Intersected(range));
    }
  }

  return valid_ranges;
}

AudioPlaybackCache::PlaybackDevice *AudioPlaybackCache::CreatePlaybackDevice(QObject* parent) const
{
  return new PlaybackDevice(playlist_, parent);
}

AudioPlaybackCache::Segment::Segment(qint64 size, const QString &filename)
{
  size_ = size;
  filename_ = filename;
}

AudioPlaybackCache::PlaybackDevice::PlaybackDevice(const AudioPlaybackCache::Playlist &playlist, QObject *parent) :
  QIODevice(parent),
  playlist_(playlist),
  current_segment_(0),
  segment_read_index_(0)
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
    QFile segment_file(cs.filename());

    if (segment_file.open(QFile::ReadOnly)) {
      // Seek to our stored index of this segment
      segment_file.seek(segment_read_index_);

      // Determine how many bytes to read
      qint64 this_read_length = qMin(current_segment_sz - segment_read_index_,
                                     maxSize - read_size);

      // Read those bytes
      segment_file.read(data + read_size, this_read_length);

      // Close the file
      segment_file.close();

      // Add to the read index
      segment_read_index_ += this_read_length;

      // Add to the read size
      read_size += this_read_length;

      // If we've reached the end of this segment, tick the counter over to the next segment
      if (segment_read_index_ == current_segment_sz) {
        // Jump to the next file
        segment_read_index_ = 0;
        current_segment_++;
      }
    } else {
      qWarning() << "Failed to read data from segment";
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

  int bottom = 0;
  int top = this->size();

  // Use a binary search to find the segment with the right offset
  while (true) {
    int mid = bottom + (top - bottom)/2;
    const Segment& mid_segment = this->at(mid);

    if (mid_segment.offset() <= pos
        && mid_segment.offset() + mid_segment.size() > pos) {
      return mid;
    }

    if (mid_segment.offset() > pos) {
      // Segment we're looking for must be lower
      top = mid;
    } else {
      // Segment we're looking for must be higher
      bottom = mid;
    }
  }
}

qint64 AudioPlaybackCache::Playlist::GetLength() const
{
  if (this->isEmpty()) {
    return 0;
  }
  return this->last().offset() + this->last().size();
}

OLIVE_NAMESPACE_EXIT
