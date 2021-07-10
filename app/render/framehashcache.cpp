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

#include "framehashcache.h"

#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfIntAttribute.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <QDir>
#include <QFileInfo>

#include "codec/frame.h"
#include "common/filefunctions.h"
#include "render/diskmanager.h"

namespace olive {

QMutex FrameHashCache::currently_saving_frames_mutex_;
QMap<QByteArray, FramePtr> FrameHashCache::currently_saving_frames_;
const QString FrameHashCache::kCacheFormatExtension = QStringLiteral(".exr");

#define super PlaybackCache

FrameHashCache::FrameHashCache(QObject *parent) :
  super(parent)
{
  if (DiskManager::instance()) {
    connect(DiskManager::instance(), &DiskManager::DeletedFrame, this, &FrameHashCache::HashDeleted);
    connect(DiskManager::instance(), &DiskManager::InvalidateProject, this, &FrameHashCache::ProjectInvalidated);
  }
}

QByteArray FrameHashCache::GetHash(const int64_t &time)
{
  if (time < GetMapSize()) {
    return time_hash_map_.at(time);
  } else {
    return QByteArray();
  }
}

QByteArray FrameHashCache::GetHash(const rational &time)
{
  return GetHash(ToTimestamp(time));
}

void FrameHashCache::SetHash(const rational &time, const QByteArray &hash, const qint64& job_time, bool frame_exists)
{
  for (int i=jobs_.size()-1; i>=0; i--) {
    const JobIdentifier& job = jobs_.at(i);

    if (job.range.Contains(time)
        && job_time < job.job_time) {
      // Hash here has changed since this frame started rendering, discard it
      return;
    }
  }

  int64_t ts = ToTimestamp(time);
  if (ts >= GetMapSize()) {
    // Reserve an extra minute to cut down on the amount of reallocations to make
    time_hash_map_.reserve(ts + timebase_.flipped().toDouble() * 60);

    // Add enough entries to insert this hash
    time_hash_map_.resize(ts + 1);
  }
  time_hash_map_[ts] = hash;

  TimeRange validated_range;
  if (frame_exists) {
    validated_range = TimeRange(time, time + timebase_);
    Validate(validated_range);
  }
}

void FrameHashCache::SetTimebase(const rational &tb)
{
  timebase_ = tb;
}

void FrameHashCache::ValidateFramesWithHash(const QByteArray &hash)
{
  const TimeRangeList& invalidated_ranges = GetInvalidatedRanges();

  for (int64_t i=0; i<GetMapSize(); i++) {
    if (time_hash_map_[i] == hash) {
      TimeRange frame_range(ToTime(i), ToTime(i+1));

      if (invalidated_ranges.contains(frame_range)) {
        Validate(frame_range);
      }
    }
  }
}

QList<rational> FrameHashCache::GetFramesWithHash(const QByteArray &hash)
{
  QList<rational> times;

  for (int64_t i=0; i<GetMapSize(); i++) {
    if (time_hash_map_.at(i) == hash) {
      times.append(ToTime(i));
    }
  }

  return times;
}

QList<rational> FrameHashCache::TakeFramesWithHash(const QByteArray &hash)
{
  TimeRangeList range_to_invalidate;
  QList<rational> times;

  for (int64_t i=0; i<GetMapSize(); i++) {
    if (time_hash_map_.at(i) == hash) {
      time_hash_map_[i].clear();

      rational time = ToTime(i);
      times.append(time);
      range_to_invalidate.insert(TimeRange(time, time + timebase_));
    }
  }

  foreach (const TimeRange& r, range_to_invalidate) {
    // We apply a 0 job time because the graph hasn't changed to get here, so any renderer should
    // be up to date already
    Invalidate(r, 0);
  }

  return times;
}

QVector<rational> FrameHashCache::GetFrameListFromTimeRange(TimeRangeList range_list, const rational &timebase)
{
  // If timebase is null, this will be an infinite loop
  Q_ASSERT(!timebase.isNull());

  QVector<rational> times;

  foreach (const TimeRange &range, range_list) {
    rational frame = Timecode::snap_time_to_timebase(range.in(), timebase, Timecode::kCeil);

    while (frame < range.out()) {
      times.append(frame);
      frame += timebase;
    }
  }

  return times;
}

QVector<rational> FrameHashCache::GetFrameListFromTimeRange(const TimeRangeList &range)
{
  return GetFrameListFromTimeRange(range, timebase_);
}

QVector<rational> FrameHashCache::GetInvalidatedFrames()
{
  return GetFrameListFromTimeRange(GetInvalidatedRanges());
}

QVector<rational> FrameHashCache::GetInvalidatedFrames(const TimeRange &intersecting)
{
  return GetFrameListFromTimeRange(GetInvalidatedRanges().Intersects(intersecting));
}

bool FrameHashCache::SaveCacheFrame(const QByteArray& hash,
                                    char* data,
                                    const VideoParams& vparam,
                                    int linesize_bytes) const
{
  return SaveCacheFrame(GetCacheDirectory(), hash, data, vparam, linesize_bytes);
}

bool FrameHashCache::SaveCacheFrame(const QByteArray &hash, FramePtr frame) const
{
  return SaveCacheFrame(GetCacheDirectory(), hash, frame);
}

bool FrameHashCache::SaveCacheFrame(const QString &cache_path, const QByteArray &hash, char *data, const VideoParams &vparam, int linesize_bytes)
{
  if (cache_path.isEmpty()) {
    qWarning() << "Failed to save cache frame with empty path";
    return false;
  }

  QString fn = CachePathName(cache_path, hash);

  if (SaveCacheFrame(fn, data, vparam, linesize_bytes)) {
    // Register frame with the disk manager
    QMetaObject::invokeMethod(DiskManager::instance(),
                              "CreatedFile",
                              Qt::QueuedConnection,
                              Q_ARG(QString, cache_path),
                              Q_ARG(QString, fn),
                              Q_ARG(QByteArray, hash));

    return true;
  } else {
    return false;
  }
}

bool FrameHashCache::SaveCacheFrame(const QString &cache_path, const QByteArray &hash, FramePtr frame)
{
  if (frame) {
    QMutexLocker locker(&currently_saving_frames_mutex_);
    currently_saving_frames_.insert(hash, frame);
    locker.unlock();

    bool ret = SaveCacheFrame(cache_path, hash, frame->data(), frame->video_params(), frame->linesize_bytes());

    locker.relock();
    currently_saving_frames_.remove(hash);
    locker.unlock();

    return ret;
  } else {
    qWarning() << "Attempted to save a NULL frame to the cache. This may or may not be desirable.";
    return false;
  }
}

FramePtr FrameHashCache::LoadCacheFrame(const QString &cache_path, const QByteArray &hash)
{
  // Minor optimization, we store frames currently being saved just in case something tries to load
  // while we're saving. This should *occasionally* optimize and also prevent scenarios where
  // we try to load a frame that's half way through being saved.
  QMutexLocker locker(&currently_saving_frames_mutex_);
  if (currently_saving_frames_.contains(hash)) {
    return currently_saving_frames_.value(hash);
  }
  locker.unlock();

  if (cache_path.isEmpty()) {
    qWarning() << "Failed to load cache frame with empty path";
    return nullptr;
  }

  return LoadCacheFrame(CachePathName(cache_path, hash));
}

FramePtr FrameHashCache::LoadCacheFrame(const QByteArray &hash) const
{
  return LoadCacheFrame(GetCacheDirectory(), hash);
}

FramePtr FrameHashCache::LoadCacheFrame(const QString &fn)
{
  FramePtr frame = nullptr;

  if (!fn.isEmpty() && QFileInfo::exists(fn)) {
    Imf::InputFile file(fn.toUtf8(), 0);

    Imath::Box2i dw = file.header().dataWindow();
    Imf::PixelType pix_type = file.header().channels().begin().channel().type;
    int width = dw.max.x - dw.min.x + 1;
    int height = dw.max.y - dw.min.y + 1;
    bool has_alpha = file.header().channels().findChannel("A");

    int div = qMax(1, static_cast<const Imf::IntAttribute&>(file.header()["oliveDivider"]).value());

    VideoParams::Format image_format;
    if (pix_type == Imf::HALF) {
      image_format = VideoParams::kFormatFloat16;
    } else {
      image_format = VideoParams::kFormatFloat32;
    }

    int channel_count = has_alpha ? VideoParams::kRGBAChannelCount : VideoParams::kRGBChannelCount;

    frame = Frame::Create();
    frame->set_video_params(VideoParams(width * div,
                                        height * div,
                                        image_format,
                                        channel_count,
                                        rational::fromDouble(file.header().pixelAspectRatio()),
                                        VideoParams::kInterlaceNone,
                                        div));

    frame->allocate();

    int bpc = VideoParams::GetBytesPerChannel(image_format);

    size_t xs = channel_count * bpc;
    size_t ys = frame->linesize_bytes();

    Imf::FrameBuffer framebuffer;
    framebuffer.insert("R", Imf::Slice(pix_type, frame->data(), xs, ys));
    framebuffer.insert("G", Imf::Slice(pix_type, frame->data() + bpc, xs, ys));
    framebuffer.insert("B", Imf::Slice(pix_type, frame->data() + 2*bpc, xs, ys));
    if (has_alpha) {
      framebuffer.insert("A", Imf::Slice(pix_type, frame->data() + 3*bpc, xs, ys));
    }

    file.setFrameBuffer(framebuffer);
    file.readPixels(dw.min.y, dw.max.y);
  }

  return frame;
}

void FrameHashCache::LengthChangedEvent(const rational &old, const rational &newlen)
{
  if (newlen < old) {
    // Determine length in frames by ceil-ing the time
    int64_t new_ts_length = ToTimestamp(newlen, Timecode::kCeil);

    // Resize vector to this length, which will discard all frames after it
    time_hash_map_.resize(new_ts_length);
  }
}

struct HashTimePair {
  rational time;
  QByteArray hash;
};

void FrameHashCache::ShiftEvent(const rational &from, const rational &to)
{
  // POSITIVE if moving forward ->
  // NEGATIVE if moving backward <-
  rational diff = to - from;
  bool diff_is_negative = (diff < 0);

  int64_t to_ts = ToTimestamp(to);
  int64_t from_ts = ToTimestamp(from);

  if (diff_is_negative) {
    // We're moving the frames starting at `from` backwards to where `to` is
    if (to_ts < GetMapSize()) {
      time_hash_map_.erase(time_hash_map_.begin() + to_ts, time_hash_map_.begin() + from_ts);
    }
  } else {
    // We're moving the frames starting at `from` forwards to where `to` is
    if (from_ts < GetMapSize()) {
      time_hash_map_.insert(time_hash_map_.begin() + from_ts, to_ts - from_ts, QByteArray());
    }
  }
}

void FrameHashCache::InvalidateEvent(const TimeRange &range)
{
  if (!timebase_.isNull()) {
    int64_t start = ToTimestamp(range.in(), Timecode::kCeil);
    int64_t end = ToTimestamp(range.out(), Timecode::kCeil);
    for (int64_t i=start; i<GetMapSize() && i<end; i++) {
      time_hash_map_[i].clear();
    }
  }
}

rational FrameHashCache::ToTime(const int64_t &ts) const
{
  return Timecode::timestamp_to_time(ts, timebase_);
}

int64_t FrameHashCache::ToTimestamp(const rational &ts, Timecode::Rounding rounding) const
{
  return Timecode::time_to_timestamp(ts, timebase_, rounding);
}

void FrameHashCache::HashDeleted(const QString& s, const QByteArray &hash)
{
  QString cache_dir = GetCacheDirectory();
  if (cache_dir.isEmpty() || s != cache_dir) {
    return;
  }

  TimeRangeList ranges_to_invalidate;
  for (int64_t i=0; i<GetMapSize(); i++) {
    if (time_hash_map_.at(i) == hash) {
      ranges_to_invalidate.insert(TimeRange(ToTime(i), ToTime(i+1)));
    }
  }

  foreach (const TimeRange& range, ranges_to_invalidate) {
    // We set job time to 0 because the nodes haven't changed and any render job should be up
    // to date
    Invalidate(range, 0);
  }
}

void FrameHashCache::ProjectInvalidated(Project *p)
{
  if (GetProject() == p) {
    time_hash_map_.clear();

    InvalidateAll();
  }
}

QString FrameHashCache::CachePathName(const QByteArray& hash) const
{
  return CachePathName(GetCacheDirectory(), hash);
}

QString FrameHashCache::CachePathName(const QString &cache_path, const QByteArray &hash)
{
  QDir cache_dir(QDir(cache_path).filePath(QString(hash.left(1).toHex())));

  QString filename = QStringLiteral("%1%2").arg(QString(hash.mid(1).toHex()), kCacheFormatExtension);

  // Register that in some way this hash has been accessed
  QMetaObject::invokeMethod(DiskManager::instance(),
                            "Accessed",
                            Qt::QueuedConnection,
                            Q_ARG(QString, cache_path),
                            Q_ARG(QByteArray, hash));

  return cache_dir.filePath(filename);
}

bool FrameHashCache::SaveCacheFrame(const QString &filename, char *data, const VideoParams &vparam, int linesize_bytes)
{
  if (!VideoParams::FormatIsFloat(vparam.format())) {
    return false;
  }

  // Ensure directory is created
  QDir cache_dir = QFileInfo(filename).dir();
  if (!cache_dir.exists()) {
    cache_dir.mkpath(".");
  }

  // Floating point types are stored in EXR
  Imf::PixelType pix_type;

  if (vparam.format() == VideoParams::kFormatFloat16) {
    pix_type = Imf::HALF;
  } else {
    pix_type = Imf::FLOAT;
  }

  Imf::Header header(vparam.effective_width(),
                     vparam.effective_height());
  header.channels().insert("R", Imf::Channel(pix_type));
  header.channels().insert("G", Imf::Channel(pix_type));
  header.channels().insert("B", Imf::Channel(pix_type));
  if (vparam.channel_count() == VideoParams::kRGBAChannelCount) {
    header.channels().insert("A", Imf::Channel(pix_type));
  }

  header.compression() = Imf::DWAA_COMPRESSION;
  header.insert("dwaCompressionLevel", Imf::FloatAttribute(200.0f));
  header.pixelAspectRatio() = vparam.pixel_aspect_ratio().toDouble();

  header.insert("oliveDivider", Imf::IntAttribute(vparam.divider()));

  Imf::OutputFile out(filename.toUtf8(), header, 0);

  int bpc = VideoParams::GetBytesPerChannel(vparam.format());

  size_t xs = vparam.channel_count() * bpc;
  size_t ys = linesize_bytes;

  Imf::FrameBuffer framebuffer;
  framebuffer.insert("R", Imf::Slice(pix_type, data, xs, ys));
  framebuffer.insert("G", Imf::Slice(pix_type, data + bpc, xs, ys));
  framebuffer.insert("B", Imf::Slice(pix_type, data + 2*bpc, xs, ys));
  if (vparam.channel_count() == VideoParams::kRGBAChannelCount) {
    framebuffer.insert("A", Imf::Slice(pix_type, data + 3*bpc, xs, ys));
  }
  out.setFrameBuffer(framebuffer);

  out.writePixels(vparam.effective_height());

  return true;
}

}
