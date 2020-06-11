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

QByteArray FrameHashCache::GetHash(const rational &time)
{
  QMutexLocker locker(lock());

  return time_hash_map_.value(time);
}

void FrameHashCache::SetHash(const rational &time, const QByteArray &hash, const qint64& job_time)
{
  QMutexLocker locker(lock());

  bool is_current = false;

  for (int i=jobs_.size()-1; i>=0; i--) {
    const JobIdentifier& job = jobs_.at(i);

    if (job.range.Contains(time)
        && job_time >= job.job_time) {
      is_current = true;
      break;
    }
  }

  if (!is_current) {
    return;
  }

  time_hash_map_.insert(time, hash);

  TimeRange validated_range(time, time + timebase_);

  NoLockValidate(validated_range);

  locker.unlock();

  emit Validated(validated_range);
}

void FrameHashCache::SetTimebase(const rational &tb)
{
  QMutexLocker locker(lock());

  timebase_ = tb;
}

QList<rational> FrameHashCache::GetFramesWithHash(const QByteArray &hash)
{
  QMutexLocker locker(lock());

  QList<rational> times;

  QMap<rational, QByteArray>::const_iterator iterator;

  for (iterator=time_hash_map_.begin();iterator!=time_hash_map_.end();iterator++) {
    if (iterator.value() == hash) {
      times.append(iterator.key());
    }
  }

  return times;
}

QList<rational> FrameHashCache::TakeFramesWithHash(const QByteArray &hash)
{
  QMutexLocker locker(lock());

  QList<rational> times;

  QMap<rational, QByteArray>::iterator iterator = time_hash_map_.begin();

  while (iterator != time_hash_map_.end()) {
    if (iterator.value() == hash) {
      times.append(iterator.key());

      iterator = time_hash_map_.erase(iterator);
    } else {
      iterator++;
    }
  }

  foreach (const rational& r, times) {
    NoLockInvalidate(TimeRange(r, r + timebase_));
  }

  locker.unlock();

  foreach (const rational& r, times) {
    emit Invalidated(TimeRange(r, r + timebase_));
  }

  return times;
}

QMap<rational, QByteArray> FrameHashCache::time_hash_map()
{
  QMutexLocker locker(lock());

  return time_hash_map_;
}

QString FrameHashCache::GetFormatExtension()
{
  return QStringLiteral(".exr");
}

QList<rational> FrameHashCache::GetFrameListFromTimeRange(TimeRangeList range_list, const rational &timebase)
{
  QList<rational> times;

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
    range_list.RemoveTimeRange(TimeRange(snapped, next));
  }

  return times;
}

QList<rational> FrameHashCache::GetFrameListFromTimeRange(const TimeRangeList &range)
{
  QMutexLocker locker(lock());

  return GetFrameListFromTimeRange(range, timebase_);
}

QList<rational> FrameHashCache::GetInvalidatedFrames()
{
  QMutexLocker locker(lock());

  return GetFrameListFromTimeRange(NoLockGetInvalidatedRanges());
}

void FrameHashCache::SaveCacheFrame(const QByteArray& hash,
                                    char* data,
                                    const VideoParams& vparam)
{
  QString fn = CachePathName(hash);

  if (SaveCacheFrame(fn, data, vparam)) {
    // Register frame with the disk manager
    DiskManager::instance()->CreatedFile(fn, hash);
  }
}

void FrameHashCache::SaveCacheFrame(const QByteArray &hash, FramePtr frame)
{
  SaveCacheFrame(hash, frame->data(), frame->video_params());
}

FramePtr FrameHashCache::LoadCacheFrame(const QByteArray &hash)
{
  return LoadCacheFrame(CachePathName(hash));
}

FramePtr FrameHashCache::LoadCacheFrame(const QString &fn)
{
  FramePtr frame = nullptr;

  if (!fn.isEmpty() && QFileInfo::exists(fn)) {
    auto input = OIIO::ImageInput::open(fn.toStdString());

    if (input) {

      PixelFormat::Format image_format = PixelFormat::OIIOFormatToOliveFormat(input->spec().format,
                                                                              input->spec().nchannels == kRGBAChannels);

      frame = Frame::Create();
      frame->set_video_params(VideoParams(input->spec().width,
                                          input->spec().height,
                                          image_format));

      frame->allocate();

      input->read_image(input->spec().format,
                        frame->data(),
                        OIIO::AutoStride,
                        frame->linesize_bytes());

      input->close();

#if OIIO_VERSION < 10903
      OIIO::ImageInput::destroy(input);
#endif

    } else {
      qWarning() << "OIIO Error:" << OIIO::geterror().c_str();
    }
  }

  return frame;
}

void FrameHashCache::LengthChangedEvent(const rational &old, const rational &newlen)
{
  if (newlen < old) {
    QMap<rational, QByteArray>::iterator i = time_hash_map_.begin();

    while (i != time_hash_map_.end()) {
      if (i.key() >= newlen) {
        i = time_hash_map_.erase(i);
      } else {
        i++;
      }
    }
  }
}

void FrameHashCache::InvalidateEvent(const TimeRange &r)
{
  QMap<rational, QByteArray>::iterator i = time_hash_map_.begin();

  while (i != time_hash_map_.end()) {
    if (i.key() >= r.in() && i.key() < r.out()) {
      i = time_hash_map_.erase(i);
    } else {
      i++;
    }
  }
}

struct HashTimePair {
  rational time;
  QByteArray hash;
};

void FrameHashCache::ShiftEvent(const rational &from, const rational &to)
{
  QMap<rational, QByteArray>::iterator i = time_hash_map_.begin();

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

QString FrameHashCache::CachePathName(const QByteArray& hash)
{
  QString ext = GetFormatExtension();

  QDir cache_dir(QDir(FileFunctions::GetMediaCacheLocation()).filePath(QString(hash.left(1).toHex())));
  cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1%2").arg(QString(hash.mid(1).toHex()), ext);

  return cache_dir.filePath(filename);
}

bool FrameHashCache::SaveCacheFrame(const QString &filename, char *data, const VideoParams &vparam)
{
  Q_ASSERT(PixelFormat::FormatIsFloat(vparam.format()));

  // Floating point types are stored in EXR
  Imf::PixelType pix_type;

  if (vparam.format() == PixelFormat::PIX_FMT_RGB16F
      || vparam.format() == PixelFormat::PIX_FMT_RGBA16F) {
    pix_type = Imf::HALF;
  } else {
    pix_type = Imf::FLOAT;
  }

  Imf::Header header(vparam.effective_width(),
                     vparam.effective_height());
  header.channels().insert("R", Imf::Channel(pix_type));
  header.channels().insert("G", Imf::Channel(pix_type));
  header.channels().insert("B", Imf::Channel(pix_type));
  if (PixelFormat::FormatHasAlphaChannel(vparam.format())) {
    header.channels().insert("A", Imf::Channel(pix_type));
  }

  header.compression() = Imf::DWAA_COMPRESSION;
  header.insert("dwaCompressionLevel", Imf::FloatAttribute(200.0f));

  Imf::OutputFile out(filename.toUtf8(), header, 0);

  int bpc = PixelFormat::BytesPerChannel(vparam.format());

  size_t xs = PixelFormat::ChannelCount(vparam.format()) * bpc;
  size_t ys = vparam.effective_width() * PixelFormat::ChannelCount(vparam.format()) * bpc;

  Imf::FrameBuffer framebuffer;
  framebuffer.insert("R", Imf::Slice(pix_type, data, xs, ys));
  framebuffer.insert("G", Imf::Slice(pix_type, data + bpc, xs, ys));
  framebuffer.insert("B", Imf::Slice(pix_type, data + 2*bpc, xs, ys));
  if (PixelFormat::FormatHasAlphaChannel(vparam.format())) {
    framebuffer.insert("A", Imf::Slice(pix_type, data + 3*bpc, xs, ys));
  }
  out.setFrameBuffer(framebuffer);

  out.writePixels(vparam.effective_height());

  return true;
}

OLIVE_NAMESPACE_EXIT
