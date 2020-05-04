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
    emit CompletedDownload(path, job_time, hash);

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
      Download(hash, path.in(), texture);
    }

    frame_cache_->RemoveHashFromCurrentlyCaching(hash);

    // Signal that this job is complete
    if (operating_mode_ & kDownloadOnly) {
      emit CompletedDownload(path, job_time, hash);
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

void VideoRenderWorker::Download(const QByteArray& hash, const rational& time, QVariant texture)
{
  if (operating_mode_ & kDownloadOnly) {

    TextureToBuffer(texture, download_buffer_.data(), 0);

    frame_cache_->SaveCacheFrame(hash,
                                 download_buffer_.data(),
                                 VideoRenderingParams(video_params_.effective_width(),
                                                      video_params_.effective_height(),
                                                      video_params_.format()));

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

    frame->set_timestamp(time);

    emit GeneratedFrame(frame);

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

ColorProcessorCache *VideoRenderWorker::color_cache()
{
  return &color_cache_;
}

OLIVE_NAMESPACE_EXIT
