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

const rational AudioPlaybackCache::kDefaultSegmentSize = 5;

AudioPlaybackCache::AudioPlaybackCache(QObject* parent) :
  PlaybackCache(parent)
{
}

AudioPlaybackCache::~AudioPlaybackCache()
{
  // Segments are volatile, so delete them here
  foreach (const Segment& s, segments_) {
    QFile::remove(s.filename());
  }
  segments_.clear();
}

void AudioPlaybackCache::SetParameters(const AudioParams &params)
{
  if (params_ == params) {
    return;
  }

  params_ = params;

  // Restart empty file so there's always "something" to play
  segments_.clear();

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
  while (segment_length_ < range.out()) {
    rational seg_sz = qMin(kDefaultSegmentSize, range.out() - segment_length_);
    segments_.push_back(CreateSegment(seg_sz));
    segment_length_ += seg_sz;
  }

  // Convert to packed data, which is what we store on disk
  QByteArray a = samples->toPackedData();

  // Keep track of validated ranges so we can signal them all at once at the end
  TimeRangeList ranges_we_validated;

  // Write each valid range to the segments
  foreach (const TimeRange& r, valid_ranges) {
    rational this_segment_in = 0;

    for (auto it=segments_.begin(); it!=segments_.end(); it++) {
      rational this_segment_out = this_segment_in + (*it).length();

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

          ranges_we_validated.InsertTimeRange(TimeRange(this_write_in_point, this_write_out_point));
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

void AudioPlaybackCache::ShiftEvent(const rational &from, const rational &to)
{
  if (from == to) {
    return;
  }

  int to_index = -1;
  int from_index = -1;
  rational to_start, from_start;

  {
    rational seg_start;

    // Find which segments intersect with this shift event
    for (int i=0; i<segments_.size(); i++) {
      const Segment& seg = segments_.at(i);

      rational seg_end = seg_start + seg.length();

      if (to_index == -1 && seg_end > to) {
        to_index = i;
        to_start = seg_start;
      }

      if (from_index == -1 && seg_end >= from) {
        from_index = i;
        from_start = seg_start;
      }

      if (to_index != -1 && from_index != -1) {
        break;
      }

      seg_start = seg_end;
    }
  }

  Segment& from_segment = segments_[from_index];
  const rational& from_length = from_segment.length();
  rational from_end = from_start + from_length;

  if (from < to) {
    // Shifting forwards, we must insert a new region and split a segment in half if necessary
    int insert_index;

    // Determine at what part of the array we'll be insert into
    if (from == from_start) {
      insert_index = from_index;
    } else {
      insert_index = from_index + 1;
    }

    if (from < from_end) {
      // Split from segment into two
      Segment second = CloneSegment(from_segment);

      TrimSegmentOut(&from_segment, from - from_start);
      TrimSegmentIn(&second, from_end - from);

      segments_.insert(insert_index, second);
    }

    // Insert silent segments
    rational time_to_insert = to - from;
    rational inserted_time;

    while (inserted_time < time_to_insert) {
      rational new_seg_sz = qMin(kDefaultSegmentSize, time_to_insert - inserted_time);

      segments_.insert(insert_index, CreateSegment(new_seg_sz));

      inserted_time += new_seg_sz;
    }

    segment_length_ += time_to_insert;

  } else {
    // Shifting backwards, we'll be removing segments and truncating them if necessary
    Segment& to_segment = segments_[to_index];
    const rational& to_length = to_segment.length();
    rational to_end = to_start + to_length;

    if (from_index == to_index) {
      // Shift occurs in the same segment
      if (to > to_start && from < to_end) {
        // Split into two and process as normal
        Segment second = CloneSegment(to_segment);
        from_index++;
        segments_.insert(from_index, second);
      } else if (to == to_start && from == to_end) {
        RemoveSegmentFromArray(to_index);
      } else if (to == to_start) {
        TrimSegmentIn(&to_segment, to_end - from);
      } else {
        TrimSegmentOut(&from_segment, to - to_start);
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
      } else if (to < to_end) {
        TrimSegmentOut(&to_segment, to - to_start);
      }

      // Remove or trim "from" segment
      if (from == from_end) {
        RemoveSegmentFromArray(from_index);
      } else if (from > from_start) {
        TrimSegmentIn(&from_segment, from_end - from);
      }
    }

    segment_length_ -= (from - to);
  }
}

void AudioPlaybackCache::LengthChangedEvent(const rational& old, const rational& newlen)
{
  if (!params_.is_valid()) {
    return;
  }

  while (newlen < segment_length_) {
    Segment& last_seg = segments_.back();

    if (segment_length_ - last_seg.length() < newlen) {
      // Truncate this segment rather than removing it
      rational diff = segment_length_ - newlen;

      TrimSegmentOut(&last_seg, last_seg.length() - diff);
      segment_length_ -= diff;
    } else {
      // Remove last segment
      segment_length_ -= last_seg.length();
      RemoveSegmentFromArray(segments_.size() - 1);
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

AudioPlaybackCache::Segment AudioPlaybackCache::CreateSegment(const rational &length) const
{
  Segment s(length, GenerateSegmentFilename());

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

void AudioPlaybackCache::TrimSegmentIn(AudioPlaybackCache::Segment *s, const rational &new_length)
{
  // Read filename
  QFile f(s->filename());
  if (f.open(QFile::ReadWrite)) {
    // Read whole segment into memory
    QByteArray data = f.readAll();

    // Trim to new length
    data = data.right(params_.time_to_bytes(new_length));

    // Clear existing file
    f.resize(0);

    // Write trimmed data
    f.write(data);

    f.close();
  }

  s->set_length(new_length);
}

void AudioPlaybackCache::TrimSegmentOut(AudioPlaybackCache::Segment *s, const rational &new_length)
{
  QFile(s->filename()).resize(params_.time_to_bytes(new_length));
  s->set_length(new_length);
}

void AudioPlaybackCache::RemoveSegmentFromArray(int index)
{
  QFile::remove(segments_.at(index).filename());
  segments_.removeAt(index);
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
  return new PlaybackDevice(segments_, parent);
}

AudioPlaybackCache::Segment::Segment(const rational &length, const QString &s)
{
  length_ = length;
  filename_ = s;
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
  // FIXME: Inefficient
  qint64 seg_start_bytes = 0;
  for (int i=0; i<playlist_.size(); i++) {
    const Segment& s = playlist_.at(i);

    qint64 this_seg_sz = QFile(s.filename()).size();
    qint64 seg_end_bytes = seg_start_bytes + this_seg_sz;

    if (pos < seg_end_bytes) {
      // This must be the segment we're looking for
      current_segment_ = i;
      segment_read_index_ = pos - seg_start_bytes;

      // Succeeded at seeking to this position
      return true;
    }

    seg_start_bytes = seg_end_bytes;
  }

  // Couldn't find this position in the file
  return false;
}

qint64 AudioPlaybackCache::PlaybackDevice::size() const
{
  qint64 p = 0;

  // FIXME: Inefficient
  for (int i=0; i<playlist_.size(); i++) {
    p += QFile(playlist_.at(i).filename()).size();
  }

  return p;
}

qint64 AudioPlaybackCache::PlaybackDevice::readData(char *data, qint64 maxSize)
{
  qint64 read_size = 0;

  while (read_size < maxSize && current_segment_ < playlist_.size()) {
    QFile segment_file(playlist_.at(current_segment_).filename());
    if (segment_file.open(QFile::ReadOnly)) {
      // Seek to our stored index of this segment
      segment_file.seek(segment_read_index_);

      // Determine how many bytes to read
      qint64 this_read_length = qMin(segment_file.size() - segment_read_index_,
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
      if (segment_read_index_ == segment_file.size()) {
        // Jump to the next file
        segment_read_index_ = 0;
        current_segment_++;
      }
    } else {
      qWarning() << "Failed to read data from segment";
    }
  }

  return read_size;
}

OLIVE_NAMESPACE_EXIT
