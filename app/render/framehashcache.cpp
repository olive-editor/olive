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
#include "render/diskmanager.h"

OLIVE_NAMESPACE_ENTER

QByteArray FrameHashCache::GetHash(const rational &time) const
{
  return time_hash_map_.value(time);
}

void FrameHashCache::SetHash(const rational &time, const QByteArray &hash)
{
  time_hash_map_.insert(time, hash);

  Validate(TimeRange(time, time + timebase_));
}

void FrameHashCache::SetTimebase(const rational &tb)
{
  timebase_ = tb;
}

QList<rational> FrameHashCache::GetFramesWithHash(const QByteArray &hash) const
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

QList<rational> FrameHashCache::TakeFramesWithHash(const QByteArray &hash)
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

const QMap<rational, QByteArray> &FrameHashCache::time_hash_map() const
{
  return time_hash_map_;
}

QString FrameHashCache::GetFormatExtension(const PixelFormat::Format &f)
{
  if (PixelFormat::FormatIsFloat(f)) {
    // EXR is only fast with float buffers so we only use it for those
    return QStringLiteral(".exr");
  } else {
    // FIXME: Will probably need different codec here. JPEG is the fastest and smallest by far (much
    //        more so than TIFF or PNG) and we don't mind lossy for the offline cache, but JPEG
    //        doesn't support >8-bit or alpha channels. JPEG2000 does, but my OIIO wasn't compiled
    //        with it and I imagine it's not common in general. Still, this works well for now as a
    //        prototype.
    return QStringLiteral(".jpg");
  }
}

void FrameHashCache::SaveCacheFrame(const QByteArray& hash,
                                    char* data,
                                    const VideoRenderingParams& vparam)
{
  QString fn = CachePathName(hash, vparam.format());

  if (SaveCacheFrame(fn, data, vparam)) {
    // Register frame with the disk manager
    DiskManager::instance()->CreatedFile(fn, hash);
  }
}

void FrameHashCache::SaveCacheFrame(const QByteArray &hash, FramePtr frame)
{
  SaveCacheFrame(hash, frame->data(), frame->video_params());
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

QString FrameHashCache::CachePathName(const QByteArray& hash, const PixelFormat::Format& pix_fmt)
{
  QString ext = GetFormatExtension(pix_fmt);

  QDir cache_dir(QDir(FileFunctions::GetMediaCacheLocation()).filePath(QString(hash.left(1).toHex())));
  cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1%2").arg(QString(hash.mid(1).toHex()), ext);

  return cache_dir.filePath(filename);
}

bool FrameHashCache::SaveCacheFrame(const QString &filename, char *data, const VideoRenderingParams &vparam)
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

      out->open(fn_std, OIIO::ImageSpec(vparam.effective_width(),
                                        vparam.effective_height(),
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
