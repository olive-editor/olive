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

#include "videorenderframecache.h"

#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <QDir>
#include <QFileInfo>

#include "codec/frame.h"
#include "common/filefunctions.h"
#include "render/diskmanager.h"

OLIVE_NAMESPACE_ENTER

void VideoRenderFrameCache::Clear()
{
  time_hash_map_.clear();

  {
    QMutexLocker locker(&currently_caching_lock_);
    currently_caching_list_.clear();
  }

  cache_id_.clear();
}

bool VideoRenderFrameCache::HasHash(const QByteArray &hash, const PixelFormat::Format& format)
{
  return QFileInfo::exists(CachePathName(hash, format)) && !IsCaching(hash);
}

bool VideoRenderFrameCache::IsCaching(const QByteArray &hash)
{
  QMutexLocker locker(&currently_caching_lock_);

  return currently_caching_list_.contains(hash);
}

bool VideoRenderFrameCache::TryCache(const QByteArray &hash)
{
  QMutexLocker locker(&currently_caching_lock_);

  if (!currently_caching_list_.contains(hash)) {
    currently_caching_list_.append(hash);
    return true;
  }

  return false;
}

void VideoRenderFrameCache::SetCacheID(const QString &id)
{
  Clear();

  cache_id_ = id;
}

QByteArray VideoRenderFrameCache::TimeToHash(const rational &time) const
{
  return time_hash_map_.value(time);
}

void VideoRenderFrameCache::SetHash(const rational &time, const QByteArray &hash)
{
  time_hash_map_.insert(time, hash);
}

void VideoRenderFrameCache::Truncate(const rational &time)
{
  QMap<rational, QByteArray>::iterator i = time_hash_map_.begin();

  while (i != time_hash_map_.end()) {
    if (i.key() >= time) {
      i = time_hash_map_.erase(i);
    } else {
      i++;
    }
  }
}

void VideoRenderFrameCache::RemoveHashFromCurrentlyCaching(const QByteArray &hash)
{
  QMutexLocker locker(&currently_caching_lock_);

  currently_caching_list_.removeOne(hash);
}

QList<rational> VideoRenderFrameCache::FramesWithHash(const QByteArray &hash) const
{
  QList<rational> times;

  QMap<rational, QByteArray>::const_iterator iterator;

  for (iterator=time_hash_map_.begin();iterator!=time_hash_map_.end();iterator++) {
    if (iterator.value() == hash) {
      times.append(iterator.key());
    }
  }

  return times;
}

QList<rational> VideoRenderFrameCache::TakeFramesWithHash(const QByteArray &hash)
{
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

  return times;
}

const QMap<rational, QByteArray> &VideoRenderFrameCache::time_hash_map() const
{
  return time_hash_map_;
}

QString VideoRenderFrameCache::GetFormatExtension(const PixelFormat::Format &f)
{
  if (PixelFormat::FormatIsFloat(f)) {
    return QStringLiteral(".exr");
  } else {
    return QStringLiteral(".jpg");
  }
}

void VideoRenderFrameCache::SaveCacheFrame(const QByteArray& hash,
                                           char* data,
                                           const VideoRenderingParams& vparam) const
{
  QString fn = CachePathName(hash, vparam.format());

  if (SaveCacheFrame(fn, data, vparam)) {
    // Register frame with the disk manager
    DiskManager::instance()->CreatedFile(fn, hash);
  }
}

QString VideoRenderFrameCache::CachePathName(const QByteArray& hash, const PixelFormat::Format& pix_fmt) const
{
  QString ext = GetFormatExtension(pix_fmt);

  QDir cache_dir(QDir(FileFunctions::GetMediaCacheLocation()).filePath(QString(hash.left(1).toHex())));
  cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1%2").arg(QString(hash.mid(1).toHex()), ext);

  return cache_dir.filePath(filename);
}

bool VideoRenderFrameCache::SaveCacheFrame(const QString &filename, char *data, const VideoRenderingParams &vparam)
{
  switch (vparam.format()) {
  case PixelFormat::PIX_FMT_RGB8:
  case PixelFormat::PIX_FMT_RGBA8:
  case PixelFormat::PIX_FMT_RGB16U:
  case PixelFormat::PIX_FMT_RGBA16U:
  {
    // Integer types are stored in JPEG which we run through OIIO

    std::string fn_std = filename.toStdString();

    auto out = OIIO::ImageOutput::create(fn_std);

    if (out) {
      // Attempt to keep this write to one thread
      out->threads(1);

      out->open(fn_std, OIIO::ImageSpec(vparam.width(),
                                        vparam.height(),
                                        PixelFormat::ChannelCount(vparam.format()),
                                        PixelFormat::GetOIIOTypeDesc(vparam.format())));

      out->write_image(PixelFormat::GetOIIOTypeDesc(vparam.format()), data);

      out->close();

#if OIIO_VERSION < 10903
      OIIO::ImageOutput::destroy(out);
#endif

      return true;
    } else {
      qCritical() << "Failed to write JPEG file:" << OIIO::geterror().c_str();
      return false;
    }
  }
  case PixelFormat::PIX_FMT_RGB16F:
  case PixelFormat::PIX_FMT_RGBA16F:
  case PixelFormat::PIX_FMT_RGB32F:
  case PixelFormat::PIX_FMT_RGBA32F:
  {
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
    header.channels().insert("A", Imf::Channel(pix_type));

    header.compression() = Imf::DWAA_COMPRESSION;
    header.insert("dwaCompressionLevel", Imf::FloatAttribute(200.0f));

    Imf::OutputFile out(filename.toUtf8(), header, 0);

    int bpc = PixelFormat::BytesPerChannel(vparam.format());

    size_t xs = kRGBAChannels * bpc;
    size_t ys = vparam.effective_width() * kRGBAChannels * bpc;

    Imf::FrameBuffer framebuffer;
    framebuffer.insert("R", Imf::Slice(pix_type, data, xs, ys));
    framebuffer.insert("G", Imf::Slice(pix_type, data + bpc, xs, ys));
    framebuffer.insert("B", Imf::Slice(pix_type, data + 2*bpc, xs, ys));
    framebuffer.insert("A", Imf::Slice(pix_type, data + 3*bpc, xs, ys));
    out.setFrameBuffer(framebuffer);

    out.writePixels(vparam.effective_height());

    return true;
  }
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    qCritical() << "Unable to cache invalid pixel format" << vparam.format();
    break;
  }

  return false;
}

OLIVE_NAMESPACE_EXIT
