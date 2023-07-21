/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include "common/oiioutils.h"
#include "render/diskmanager.h"

namespace olive {

#define super PlaybackCache

FrameHashCache::FrameHashCache(QObject *parent) :
  super(parent)
{
  if (DiskManager::instance()) {
    connect(DiskManager::instance(), &DiskManager::DeletedFrame, this, &FrameHashCache::HashDeleted);
    connect(DiskManager::instance(), &DiskManager::InvalidateProject, this, &FrameHashCache::ProjectInvalidated);
  }
}

void FrameHashCache::SetTimebase(const rational &tb)
{
  timebase_ = tb;
}

void FrameHashCache::ValidateTimestamp(const int64_t &ts)
{
  TimeRange frame_range(ToTime(ts), ToTime(ts+1));
  Validate(frame_range);
}

void FrameHashCache::ValidateTime(const rational &time)
{
  Validate(TimeRange(time, time + timebase_));
}

QString FrameHashCache::GetValidCacheFilename(const rational &time) const
{
  if (IsFrameCached(time)) {
    return CachePathName(time);
  } else if (!GetPassthroughs().empty()) {
    for (const Passthrough &p : GetPassthroughs()) {
      if (p.Contains(time)) {
        return CachePathName(GetCacheDirectory(), p.cache, time, timebase_);
      }
    }
  }

  return QString();
}

bool FrameHashCache::SaveCacheFrame(const int64_t &time, FramePtr frame) const
{
  return SaveCacheFrame(GetCacheDirectory(), GetUuid(), time, frame);
}

bool FrameHashCache::SaveCacheFrame(const QString &cache_path, const QUuid &uuid, const int64_t &time, FramePtr frame)
{
  if (cache_path.isEmpty()) {
    qWarning() << "Failed to save cache frame with empty path";
    return false;
  }

  QString fn = CachePathName(cache_path, uuid, time);

  bool ret = SaveCacheFrame(fn, frame);

  // Register frame with the disk manager
  if (ret) {
    QMetaObject::invokeMethod(DiskManager::instance(), "CreatedFile", Q_ARG(QString, cache_path), Q_ARG(QString, fn));
  }

  return ret;
}

bool FrameHashCache::SaveCacheFrame(const QString &cache_path, const QUuid &uuid, const rational &time, const rational &tb, FramePtr frame)
{
  if (cache_path.isEmpty()) {
    qWarning() << "Failed to save cache frame with empty path";
    return false;
  }

  QString fn = CachePathName(cache_path, uuid, time, tb);

  bool ret = SaveCacheFrame(fn, frame);

  // Register frame with the disk manager
  if (ret) {
    QMetaObject::invokeMethod(DiskManager::instance(), "CreatedFile", Q_ARG(QString, cache_path), Q_ARG(QString, fn));
  }

  return ret;
}

FramePtr FrameHashCache::LoadCacheFrame(const QString &cache_path, const QUuid &uuid, const int64_t &time)
{
  // Minor optimization, we store frames currently being saved just in case something tries to load
  // while we're saving. This should *occasionally* optimize and also prevent scenarios where
  // we try to load a frame that's half way through being saved.
  QString filename = CachePathName(cache_path, uuid, time);

  if (cache_path.isEmpty()) {
    qWarning() << "Failed to load cache frame with empty path";
    return nullptr;
  }

  return LoadCacheFrame(filename);
}

FramePtr FrameHashCache::LoadCacheFrame(const int64_t &hash) const
{
  return LoadCacheFrame(GetCacheDirectory(), GetUuid(), hash);
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

      PixelFormat image_format;
      if (pix_type == Imf::HALF) {
        image_format = PixelFormat::F16;
      } else {
        image_format = PixelFormat::F32;
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
      // Not an EXR, maybe it's a JPEG?
      QImage img;

      if (img.load(fn, "jpg")) {

        // FIXME: Hardcoded
        const int div = 1;
        const PixelFormat image_format = PixelFormat::U8;
        const int channel_count = 4;
        const rational par(1, 1);

        // Convert to frame (FIXME: might be slow? may be a better way to do this on the GPU)
        img.convertTo(QImage::Format_RGBA8888_Premultiplied);

        frame = Frame::Create();
        frame->set_video_params(VideoParams(img.width() * div,
                                            img.height() * div,
                                            image_format,
                                            channel_count,
                                            par,
                                            VideoParams::kInterlaceNone,
                                            div));

        frame->allocate();

        for (int i=0; i<img.height(); i++) {
          memcpy(frame->data() + frame->linesize_bytes() * i,
                 img.bits() + img.bytesPerLine() * i,
                 frame->width() * frame->video_params().GetBytesPerPixel());
        }

      } else {
        qCritical() << "Failed to read cache frame:" << e.what();

        // Clear frame to signal that nothing was loaded
        frame = nullptr;

        // Assume this frame is corrupt in some way and delete it
        QMetaObject::invokeMethod(DiskManager::instance(), "DeleteSpecificFile", Q_ARG(QString, fn));
      }
    }

  }

  return frame;
}

void FrameHashCache::SetPassthrough(PlaybackCache *cache)
{
  super::SetPassthrough(cache);
  SetTimebase(static_cast<FrameHashCache*>(cache)->GetTimebase());
}

void FrameHashCache::LoadStateEvent(QDataStream &stream)
{
  uint32_t version;
  int num, den;

  stream >> version;

  switch (version) {
  case 1:
    stream >> num;
    stream >> den;
    timebase_ = rational(num, den);
    break;
  }
}

void FrameHashCache::SaveStateEvent(QDataStream &stream)
{
  uint32_t version = 1;

  stream << version;

  stream << timebase_.numerator();
  stream << timebase_.denominator();
}

rational FrameHashCache::ToTime(const int64_t &ts) const
{
  return Timecode::timestamp_to_time(ts, timebase_);
}

int64_t FrameHashCache::ToTimestamp(const rational &ts, Timecode::Rounding rounding) const
{
  return Timecode::time_to_timestamp(ts, timebase_, rounding);
}

void FrameHashCache::HashDeleted(const QString& path, const QString &filename)
{
  QString cache_dir = GetCacheDirectory();
  if (cache_dir.isEmpty() || path != cache_dir) {
    return;
  }

  QFileInfo info(filename);
  if (GetUuid().toString() != info.dir().dirName()) {
    return;
  }

  int64_t timestamp = info.fileName().toLongLong();
  Invalidate(TimeRange(ToTime(timestamp), ToTime(timestamp + 1)));
}

void FrameHashCache::ProjectInvalidated(Project *p)
{
  if (GetProject() == p) {
    InvalidateAll();
  }
}

QString FrameHashCache::CachePathName(const int64_t &time) const
{
  return CachePathName(GetCacheDirectory(), GetUuid(), time);
}

QString FrameHashCache::CachePathName(const rational &time) const
{
  return CachePathName(GetCacheDirectory(), GetUuid(), time, timebase_);
}

QString FrameHashCache::CachePathName(const QString &cache_path, const QUuid &cache_id, const int64_t &time)
{
  QString filename = GetThisCacheDirectory(cache_path, cache_id).filePath(QString::number(time));

  // Register that in some way this hash has been accessed
  if (DiskManager::instance()) {
    QMetaObject::invokeMethod(DiskManager::instance(), "Accessed", Q_ARG(QString, cache_path), Q_ARG(QString, filename));
  }

  return filename;
}

QString FrameHashCache::CachePathName(const QString &cache_path, const QUuid &cache_id, const rational &time, const rational &tb)
{
  return CachePathName(cache_path, cache_id, Timecode::time_to_timestamp(time, tb, Timecode::kRound));
}

bool FrameHashCache::SaveCacheFrame(const QString &filename, const FramePtr frame)
{
  // Ensure directory is created
  QDir cache_dir = QFileInfo(filename).dir();
  if (!FileFunctions::DirectoryIsValid(cache_dir)) {
    return false;
  }

  if (VideoParams::FormatIsFloat(frame->format())) {
    // Floating point types are stored in EXR
    Imf::PixelType pix_type;

    if (frame->format() == PixelFormat::F16) {
      pix_type = Imf::HALF;
    } else {
      pix_type = Imf::FLOAT;
    }

    Imf::Header header(frame->width(), frame->height());
    header.channels().insert("R", Imf::Channel(pix_type));
    header.channels().insert("G", Imf::Channel(pix_type));
    header.channels().insert("B", Imf::Channel(pix_type));
    if (frame->channel_count() == VideoParams::kRGBAChannelCount) {
      header.channels().insert("A", Imf::Channel(pix_type));
    }

    header.compression() = Imf::DWAA_COMPRESSION;
    header.insert("dwaCompressionLevel", Imf::FloatAttribute(200.0f));
    header.pixelAspectRatio() = frame->video_params().pixel_aspect_ratio().toDouble();

    header.insert("oliveDivider", Imf::IntAttribute(frame->video_params().divider()));

    try {
      Imf::OutputFile out(filename.toUtf8(), header, 0);

      int bpc = VideoParams::GetBytesPerChannel(frame->format());

      size_t xs = frame->channel_count() * bpc;
      size_t ys = frame->linesize_bytes();

      Imf::FrameBuffer framebuffer;
      framebuffer.insert("R", Imf::Slice(pix_type, frame->data(), xs, ys));
      framebuffer.insert("G", Imf::Slice(pix_type, frame->data() + bpc, xs, ys));
      framebuffer.insert("B", Imf::Slice(pix_type, frame->data() + 2*bpc, xs, ys));
      if (frame->channel_count() == VideoParams::kRGBAChannelCount) {
        framebuffer.insert("A", Imf::Slice(pix_type, frame->data() + 3*bpc, xs, ys));
      }
      out.setFrameBuffer(framebuffer);

      out.writePixels(frame->height());

      return true;
    } catch (const std::exception &e) {
      qCritical() << "Failed to write cache frame:" << e.what();

      return false;
    }
  } else {
    QImage::Format fmt = QImage::Format_Invalid;

    switch (frame->format()) {
    case PixelFormat::U8:
      if (frame->channel_count() == VideoParams::kRGBAChannelCount){
        fmt = QImage::Format_RGBA8888_Premultiplied;
      } else if (frame->channel_count() == VideoParams::kRGBChannelCount){
        fmt = QImage::Format_RGB888;
      }
      break;
    case PixelFormat::U16:
      if (frame->channel_count() == VideoParams::kRGBAChannelCount){
        fmt = QImage::Format_RGBA64_Premultiplied;
      }
      break;
    case PixelFormat::F16:
    case PixelFormat::F32:
    case PixelFormat::COUNT:
    case PixelFormat::INVALID:
      break;
    }

    if (fmt == QImage::Format_Invalid) {
      return false;
    }

    QImage img(reinterpret_cast<const uchar*>(frame->data()), frame->width(), frame->height(), frame->linesize_bytes(), fmt);

    return img.save(filename, "jpg");
  }
}

}
