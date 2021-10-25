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

void FrameHashCache::SetHash(const rational &time, const QByteArray &hash, bool frame_exists)
{
  int64_t ts = ToTimestamp(time);
  if (ts >= GetMapSize()) {
    // Disabled: bizarrely causes the whole app to hang indefinitely when used
    // Reserve an extra minute to cut down on the amount of reallocations to make
    //time_hash_map_.reserve(ts + timebase_.flipped().toDouble() * 60);

    // Add enough entries to insert this hash
    time_hash_map_.resize(ts + 1);
  }
  time_hash_map_[ts] = hash;
  hash_time_map_[hash].push_back(ts);

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
  auto invalidated_ranges = GetInvalidatedRanges(ToTime(GetMapSize()));

  const std::vector<int64_t> &times = hash_time_map_[hash];
  for (auto it=times.cbegin(); it!=times.cend(); it++) {
    const int64_t &i = *it;

    TimeRange frame_range(ToTime(i), ToTime(i+1));

    if (invalidated_ranges.contains(frame_range)) {
      Validate(frame_range);
    }
  }
}

bool FrameHashCache::SaveCacheFrame(const QByteArray& hash,
                                    char* data,
                                    const VideoParams& vparam,
                                    int linesize_bytes) const
{
  return SaveCacheFrame(GetCacheDirectory(), hash, data, vparam, linesize_bytes);
}

bool FrameHashCache::SaveCacheFrames(QVector<QByteArray> hashes, QVector<FramePtr> frames) const
{
  return SaveCacheFrames(GetCacheDirectory(), hashes, frames);
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

bool FrameHashCache::SaveCacheFrames(const QString &cache_path, QVector<QByteArray> hashes, QVector<FramePtr> frames)
{
  if (frames.isEmpty()) {
    qWarning() << "Attempted to save a NULL frame to the cache. This may or may not be desirable.";
    return false;
  }

  QMutexLocker locker(&currently_saving_frames_mutex_);

  bool ret = true;

  for (int i=0; i<hashes.count(); i++){
    currently_saving_frames_.insert(hashes[i], frames[i]);
    locker.unlock();

    ret = ret && SaveCacheFrame(cache_path, hashes[i], frames[i]->data(), frames[i]->video_params(), frames[i]->linesize_bytes());

    locker.relock();
    currently_saving_frames_.remove(hashes[i]);
    locker.unlock();
  }

  return ret;
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
    try {
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
    } catch (const std::exception &e) {
      qCritical() << "Failed to read cache frame:" << e.what();

      // Clear frame to signal that nothing was loaded
      frame = nullptr;

      // Assume this frame is corrupt in some way and delete it
      QMetaObject::invokeMethod(DiskManager::instance(), "DeleteSpecificFile", Qt::QueuedConnection, Q_ARG(QString, fn));
    }

  }

  return frame;
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

  if (from_ts >= GetMapSize()) {
    return;
  }

  int64_t ts_diff = to_ts - from_ts;
  for (int64_t i=qMin(from_ts, to_ts); i<GetMapSize(); i++) {
    const QByteArray &hash = time_hash_map_[i];
    std::vector<int64_t> &times = hash_time_map_[hash];

    for (auto jt=times.begin(); jt!=times.end(); jt++) {
      int64_t &this_ts = *jt;
      if (this_ts == i) {
        this_ts += ts_diff;
        break;
      }
    }
  }

  if (diff_is_negative) {
    // We're moving the frames starting at `from` backwards to where `to` is
    if (to_ts < GetMapSize()) {
      time_hash_map_.erase(time_hash_map_.begin() + to_ts, time_hash_map_.begin() + from_ts);
    }
  } else {
    // We're moving the frames starting at `from` forwards to where `to` is
    if (from_ts < GetMapSize()) {
      time_hash_map_.insert(time_hash_map_.begin() + from_ts, ts_diff, QByteArray());
    }
  }
}

void FrameHashCache::InvalidateEvent(const TimeRange &range)
{
  if (!timebase_.isNull()) {
    int64_t start = ToTimestamp(range.in(), Timecode::kCeil);
    int64_t end = ToTimestamp(range.out(), Timecode::kCeil);
    for (int64_t i=start; i<GetMapSize() && i<end; i++) {
      QByteArray &hash = time_hash_map_[i];

      std::vector<int64_t> &times_for_hash = hash_time_map_[hash];
      for (auto it=times_for_hash.cbegin(); it!=times_for_hash.cend(); ) {
        if ((*it) == i) {
          times_for_hash.erase(it);
          break;
        } else {
          it++;
        }
      }

      hash.clear();
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
  const std::vector<int64_t> &times_for_hash = hash_time_map_[hash];

  for (auto it=times_for_hash.begin(); it!=times_for_hash.end(); it++) {
    const int64_t &i = *it;
    ranges_to_invalidate.insert(TimeRange(ToTime(i), ToTime(i+1)));
  }

  foreach (const TimeRange& range, ranges_to_invalidate) {
    // We set job time to 0 because the nodes haven't changed and any render job should be up
    // to date
    Invalidate(range);
  }
}

void FrameHashCache::ProjectInvalidated(Project *p)
{
  if (GetProject() == p) {
    time_hash_map_.clear();
    hash_time_map_.clear();

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

  try {
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
  } catch (const std::exception &e) {
    qCritical() << "Failed to write cache frame:" << e.what();

    return false;
  }
}

}
