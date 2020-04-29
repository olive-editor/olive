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

#include "videorenderworker.h"

#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>

#include "common/define.h"
#include "common/functiontimer.h"
#include "node/block/transition/transition.h"
#include "node/node.h"
#include "project/project.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

VideoRenderWorker::VideoRenderWorker(VideoRenderFrameCache *frame_cache, QObject *parent) :
  RenderWorker(parent),
  frame_cache_(frame_cache),
  operating_mode_(kHashRenderCache)
{
}

const VideoRenderingParams &VideoRenderWorker::video_params()
{
  return video_params_;
}

void VideoRenderWorker::TextureToBuffer(const QVariant &texture, void *buffer, int linesize)
{
  TextureToBuffer(texture,
                  video_params_.effective_width(),
                  video_params_.effective_height(),
                  QMatrix4x4(),
                  buffer,
                  linesize);
}

NodeValueTable VideoRenderWorker::RenderInternal(const NodeDependency& path, const qint64 &job_time)
{
  // Get hash of node graph
  // We use SHA-1 for speed (benchmarks show it's the fastest hash available to us)
  QByteArray hash;
  if (operating_mode_ & kHashOnly) {
    QCryptographicHash hasher(QCryptographicHash::Sha1);

    // Embed video parameters into this hash
    int vwidth = video_params_.effective_width();
    int vheight = video_params_.effective_height();
    PixelFormat::Format vfmt = video_params_.format();
    RenderMode::Mode vmode = video_params_.mode();

    hasher.addData(reinterpret_cast<const char*>(&vwidth), sizeof(int));
    hasher.addData(reinterpret_cast<const char*>(&vheight), sizeof(int));
    hasher.addData(reinterpret_cast<const char*>(&vfmt), sizeof(PixelFormat::Format));
    hasher.addData(reinterpret_cast<const char*>(&vmode), sizeof(RenderMode::Mode));

    path.node()->Hash(hasher, path.in());
    hash = hasher.result();
  }

  NodeValueTable value;

  if (!(operating_mode_ & kRenderOnly)) {

    // Emit only the hash
    emit CompletedDownload(path, job_time, hash, false);

  } else if ((operating_mode_ & kHashOnly) && frame_cache_->HasHash(hash, video_params_.format())) {

    // We've already cached this hash, no need to continue
    emit HashAlreadyExists(path, job_time, hash);

  } else if (!(operating_mode_ & kHashOnly) || frame_cache_->TryCache(hash)) {

    // This hash is available for us to cache, start traversing graph
    value = ProcessNode(path);

    // Find texture in hash
    QVariant texture = value.Get(NodeParam::kTexture);

    // If we actually have a texture, download it into the disk cache
    if (!texture.isNull() || (!(operating_mode_ & kDownloadOnly))) {
      Download(path.in(), texture, frame_cache_->CachePathName(hash, video_params_.format()));
    }

    frame_cache_->RemoveHashFromCurrentlyCaching(hash);

    // Signal that this job is complete
    if (operating_mode_ & kDownloadOnly) {
      emit CompletedDownload(path, job_time, hash, !texture.isNull());
    }

  } else {

    // Another thread must be caching this already, nothing to be done
    emit HashAlreadyBeingCached(path, job_time, hash);

  }

  return value;
}

void VideoRenderWorker::SetParameters(const VideoRenderingParams &video_params)
{
  video_params_ = video_params;

  if (IsStarted()) {
    ResizeDownloadBuffer();
  }

  ParametersChangedEvent();
}

void VideoRenderWorker::SetOperatingMode(const VideoRenderWorker::OperatingMode &mode)
{
  operating_mode_ = mode;
}

void VideoRenderWorker::SetFrameGenerationParams(int width, int height, const QMatrix4x4 &matrix)
{
  frame_gen_params_ = VideoRenderingParams(width,
                                           height,
                                           video_params_.time_base(),
                                           video_params_.format(),
                                           video_params_.mode(),
                                           video_params_.divider());
  frame_gen_mat_ = matrix;
}

bool VideoRenderWorker::InitInternal()
{
  if (video_params_.is_valid()) {
    ResizeDownloadBuffer();
  }

  return true;
}

void VideoRenderWorker::CloseInternal()
{
  download_buffer_.clear();
}

void VideoRenderWorker::Download(const rational& time, QVariant texture, QString filename)
{
  if (operating_mode_ & kDownloadOnly) {

    TextureToBuffer(texture, download_buffer_.data(), 0);

    switch (video_params().format()) {
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

        out->open(fn_std, OIIO::ImageSpec(video_params().effective_width(),
                                          video_params().effective_height(),
                                          PixelFormat::ChannelCount(video_params().format()),
                                          PixelFormat::GetOIIOTypeDesc(video_params().format())));

        out->write_image(PixelFormat::GetOIIOTypeDesc(video_params().format()), download_buffer_.data());

        out->close();

#if OIIO_VERSION < 10903
        OIIO::ImageOutput::destroy(out);
#endif
      } else {
        qCritical() << "Failed to write JPEG file:" << OIIO::geterror().c_str();
      }
      break;
    }
    case PixelFormat::PIX_FMT_RGB16F:
    case PixelFormat::PIX_FMT_RGBA16F:
    case PixelFormat::PIX_FMT_RGB32F:
    case PixelFormat::PIX_FMT_RGBA32F:
    {
      // Floating point types are stored in EXR
      Imf::PixelType pix_type;

      if (video_params().format() == PixelFormat::PIX_FMT_RGB16F
          || video_params().format() == PixelFormat::PIX_FMT_RGBA16F) {
        pix_type = Imf::HALF;
      } else {
        pix_type = Imf::FLOAT;
      }

      Imf::Header header(video_params().effective_width(),
                         video_params().effective_height());
      header.channels().insert("R", Imf::Channel(pix_type));
      header.channels().insert("G", Imf::Channel(pix_type));
      header.channels().insert("B", Imf::Channel(pix_type));
      header.channels().insert("A", Imf::Channel(pix_type));

      header.compression() = Imf::DWAA_COMPRESSION;
      header.insert("dwaCompressionLevel", Imf::FloatAttribute(200.0f));

      Imf::OutputFile out(filename.toUtf8(), header, 0);

      int bpc = PixelFormat::BytesPerChannel(video_params().format());

      size_t xs = kRGBAChannels * bpc;
      size_t ys = video_params().effective_width() * kRGBAChannels * bpc;

      Imf::FrameBuffer framebuffer;
      framebuffer.insert("R", Imf::Slice(pix_type, download_buffer_.data(), xs, ys));
      framebuffer.insert("G", Imf::Slice(pix_type, download_buffer_.data() + bpc, xs, ys));
      framebuffer.insert("B", Imf::Slice(pix_type, download_buffer_.data() + 2*bpc, xs, ys));
      framebuffer.insert("A", Imf::Slice(pix_type, download_buffer_.data() + 3*bpc, xs, ys));
      out.setFrameBuffer(framebuffer);

      out.writePixels(video_params().effective_height());
      break;
    }
    case PixelFormat::PIX_FMT_INVALID:
    case PixelFormat::PIX_FMT_COUNT:
      qCritical() << "Unable to cache invalid pixel format" << video_params().format();
      break;
    }

  } else {

    FramePtr frame = Frame::Create();

    if (frame_gen_params_.is_valid()) {
      frame->set_video_params(frame_gen_params_);
    } else {
      frame->set_video_params(VideoRenderingParams(video_params_.effective_width(),
                                                   video_params_.effective_height(),
                                                   video_params_.format()));
    }

    frame->allocate();

    if (texture.isNull()) {
      memset(frame->data(), 0, frame->allocated_size());
    } else {
      TextureToBuffer(texture, frame->width(), frame->height(), frame_gen_mat_, frame->data(), frame->linesize_pixels());
    }

    emit GeneratedFrame(time, frame);

  }
}

void VideoRenderWorker::ResizeDownloadBuffer()
{
  download_buffer_.resize(PixelFormat::GetBufferSize(video_params_.format(), video_params_.effective_width(), video_params_.effective_height()));
}

NodeValueTable VideoRenderWorker::RenderBlock(const TrackOutput *track, const TimeRange &range)
{
  // A frame can only have one active block so we just validate the in point of the range
  Block* active_block = track->BlockAtTime(range.in());

  NodeValueTable table;

  if (active_block) {
    table = ProcessNode(NodeDependency(active_block, range));
  }

  return table;
}

void VideoRenderWorker::ReportUnavailableFootage(StreamPtr stream, Decoder::RetrieveState state, const rational &stream_time)
{
  emit FootageUnavailable(stream,
                          state,
                          TimeRange(CurrentPath().in(), CurrentPath().in() + video_params().time_base()),
                          stream_time);
}

ColorProcessorCache *VideoRenderWorker::color_cache()
{
  return &color_cache_;
}

OLIVE_NAMESPACE_EXIT
