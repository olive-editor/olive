/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <QDir>
#include <QFileInfo>

#include "codec/frame.h"
#include "common/filefunctions.h"
#include "common/timecodefunctions.h"
#include "render/diskmanager.h"

OLIVE_NAMESPACE_ENTER

FrameHashCache::FrameHashCache(QObject *parent) :
  PlaybackCache(parent)
{
  if (DiskManager::instance()) {
    connect(DiskManager::instance(), &DiskManager::DeletedFrame, this, &FrameHashCache::HashDeleted);
    connect(DiskManager::instance(), &DiskManager::InvalidateProject, this, &FrameHashCache::ProjectInvalidated);
  }
}

QByteArray FrameHashCache::GetHash(const rational &time)
{
  return time_hash_map_.value(time);
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

  time_hash_map_.insert(time, hash);

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

  for (auto iterator=time_hash_map_.begin();iterator!=time_hash_map_.end();iterator++) {
    if (iterator.value() == hash) {
      TimeRange frame_range(iterator.key(), iterator.key() + timebase_);

      if (invalidated_ranges.contains(frame_range)) {
        Validate(frame_range);
      }
    }
  }
}

QList<rational> FrameHashCache::GetFramesWithHash(const QByteArray &hash)
{
  QList<rational> times;

  for (auto iterator=time_hash_map_.begin();iterator!=time_hash_map_.end();iterator++) {
    if (iterator.value() == hash) {
      times.append(iterator.key());
    }
  }

  return times;
}

QList<rational> FrameHashCache::TakeFramesWithHash(const QByteArray &hash)
{
  QList<rational> times;

  auto iterator = time_hash_map_.begin();

  while (iterator != time_hash_map_.end()) {
    if (iterator.value() == hash) {
      times.append(iterator.key());

      iterator = time_hash_map_.erase(iterator);
    } else {
      iterator++;
    }
  }

  foreach (const rational& r, times) {
    Invalidate(TimeRange(r, r + timebase_));
  }

  return times;
}

QMap<rational, QByteArray> FrameHashCache::time_hash_map()
{
  return time_hash_map_;
}

QString FrameHashCache::GetFormatExtension()
{
  return QStringLiteral(".exr");
}

QVector<rational> FrameHashCache::GetFrameListFromTimeRange(TimeRangeList range_list, const rational &timebase)
{
  // If timebase is null, this will be an infinite loop
  Q_ASSERT(!timebase.isNull());

  QVector<rational> times;

  while (!range_list.isEmpty()) {
    const TimeRange& range = range_list.first();

    rational time = range.in();
    rational snapped = Timecode::snap_time_to_timebase(time, timebase);
    rational next;

    if (snapped > time) {
      next = snapped;
      snapped -= timebase;
    } else {
      next = snapped + timebase;
    }

    times.append(snapped);
    range_list.remove(TimeRange(snapped, next));
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
  QString fn = CachePathName(hash);

  if (SaveCacheFrame(fn, data, vparam, linesize_bytes)) {
    // Register frame with the disk manager
    QMetaObject::invokeMethod(DiskManager::instance(),
                              "CreatedFile",
                              Qt::QueuedConnection,
                              Q_ARG(QString, GetCacheDirectory()),
                              Q_ARG(QString, fn),
                              Q_ARG(QByteArray, hash));

    return true;
  } else {
    return false;
  }
}

bool FrameHashCache::SaveCacheFrame(const QByteArray &hash, FramePtr frame) const
{
  if (frame) {
    return SaveCacheFrame(hash, frame->data(), frame->video_params(), frame->linesize_bytes());
  } else {
    qWarning() << "Attempted to save a NULL frame to the cache. This may or may not be desirable.";
    return false;
  }
}

FramePtr FrameHashCache::LoadCacheFrame(const QString &cache_path, const QByteArray &hash)
{
  return LoadCacheFrame(CachePathName(cache_path, hash));
}

FramePtr FrameHashCache::LoadCacheFrame(const QByteArray &hash) const
{
  return LoadCacheFrame(CachePathName(hash));
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

    VideoParams::Format image_format;
    if (pix_type == Imf::HALF) {
      image_format = VideoParams::kFormatFloat16;
    } else {
      image_format = VideoParams::kFormatFloat32;
    }

    int channel_count = has_alpha ? VideoParams::kRGBAChannelCount : VideoParams::kRGBChannelCount;

    frame = Frame::Create();
    frame->set_video_params(VideoParams(width,
                                        height,
                                        image_format,
                                        channel_count,
                                        rational::fromDouble(file.header().pixelAspectRatio())));

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
    auto i = time_hash_map_.begin();

    while (i != time_hash_map_.end()) {
      if (i.key() >= newlen) {
        i = time_hash_map_.erase(i);
      } else {
        i++;
      }
    }
  }
}

struct HashTimePair {
  rational time;
  QByteArray hash;
};

void FrameHashCache::ShiftEvent(const rational &from, const rational &to)
{
  auto i = time_hash_map_.begin();

  // POSITIVE if moving forward ->
  // NEGATIVE if moving backward <-
  rational diff = to - from;
  bool diff_is_negative = (diff < rational());

  QList<HashTimePair> shifted_times;

  while (i != time_hash_map_.end()) {
    if (diff_is_negative && i.key() >= to && i.key() < from) {

      // This time will be removed in the shift so we just discard it
      i = time_hash_map_.erase(i);

    } else if (i.key() >= from) {

      // This time is after the from time and must be shifted
      shifted_times.append({i.key() + diff, i.value()});
      i = time_hash_map_.erase(i);

    } else {

      // Do nothing
      i++;

    }
  }

  foreach (const HashTimePair& p, shifted_times) {
    time_hash_map_.insert(p.time, p.hash);
  }
}

void FrameHashCache::InvalidateEvent(const TimeRange &range)
{
  QVector<rational> invalid_frames = GetFrameListFromTimeRange({range});

  foreach (const rational& r, invalid_frames) {
    time_hash_map_.remove(r);
  }
}

void FrameHashCache::HashDeleted(const QString& s, const QByteArray &hash)
{
  QString cache_dir = GetCacheDirectory();
  if (cache_dir.isEmpty() || s != cache_dir) {
    return;
  }

  TimeRangeList ranges_to_invalidate;
  for (auto i=time_hash_map_.constBegin(); i!=time_hash_map_.constEnd(); i++) {
    if (i.value() == hash) {
      ranges_to_invalidate.insert(TimeRange(i.key(), i.key() + timebase_));
    }
  }

  foreach (const TimeRange& range, ranges_to_invalidate) {
    Invalidate(range);
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
  QString ext = GetFormatExtension();

  QDir cache_dir(QDir(cache_path).filePath(QString(hash.left(1).toHex())));
  cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1%2").arg(QString(hash.mid(1).toHex()), ext);

  // Register that in some way this hash has been accessed
  QMetaObject::invokeMethod(DiskManager::instance(),
                            "Accessed",
                            Qt::QueuedConnection,
                            Q_ARG(QString, cache_path),
                            Q_ARG(QByteArray, hash));

  return cache_dir.filePath(filename);
}

bool FrameHashCache::SaveCacheFrame(const QString &filename, char *data, const VideoParams &vparam, int linesize_bytes) const
{
  if (!VideoParams::FormatIsFloat(vparam.format())) {
    qCritical() << "Tried to cache frame with non-float pixel format";
    return false;
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

OLIVE_NAMESPACE_EXIT
