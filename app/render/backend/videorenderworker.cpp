#include "videorenderworker.h"

#include "common/define.h"
#include "node/block/transition/transition.h"
#include "node/node.h"
#include "project/project.h"
#include "render/pixelservice.h"

VideoRenderWorker::VideoRenderWorker(VideoRenderFrameCache *frame_cache, QObject *parent) :
  RenderWorker(parent),
  frame_cache_(frame_cache)
{

}

const VideoRenderingParams &VideoRenderWorker::video_params()
{
  return video_params_;
}

NodeValueTable VideoRenderWorker::RenderInternal(const NodeDependency& path, const qint64 &job_time)
{
  // Get hash of node graph
  // We use SHA-1 for speed (benchmarks show it's the fastest hash available to us)
  QCryptographicHash hasher(QCryptographicHash::Sha1);
  HashNodeRecursively(&hasher, path.node(), path.in());
  QByteArray hash = hasher.result();

  NodeValueTable value;

  if (frame_cache_->HasHash(hash)) {
    // We've already cached this hash, no need to continue
    emit HashAlreadyExists(path, job_time, hash);
  } else if (frame_cache_->TryCache(path.in(), hash)) {
    // This hash is available for us to cache, start traversing graph
    value = ProcessNode(path);

    // Find texture in hash
    QVariant texture = value.Get(NodeParam::kTexture);

    // Signal that we have a frame in memory that could be shown right now
    emit CompletedFrame(path, job_time, hash, texture);

    // If we actually have a texture, download it into the disk cache
    if (!texture.isNull()) {
      Download(path, hash, texture, frame_cache_->CachePathName(hash));
    }

    frame_cache_->RemoveHashFromCurrentlyCaching(hash);

    // Signal that this job is complete
    emit CompletedDownload(path, job_time, hash);
  } else {
    // Another thread must be caching this already, nothing to be done
    emit HashAlreadyBeingCached(path, job_time, hash);
  }

  return value;
}

FramePtr VideoRenderWorker::RetrieveFromDecoder(DecoderPtr decoder, const TimeRange &range)
{
  return decoder->RetrieveVideo(range.in());
}

void VideoRenderWorker::HashNodeRecursively(QCryptographicHash *hash, const Node* n, const rational& time)
{
  // Resolve BlockList
  if (n->IsTrack()) {
    n = static_cast<const TrackOutput*>(n)->BlockAtTime(time);

    if (!n) {
      return;
    }
  }

  // Add this Node's ID
  hash->addData(n->id().toUtf8());

  if (n->IsBlock() && static_cast<const Block*>(n)->type() == Block::kTransition) {
    const TransitionBlock* transition = static_cast<const TransitionBlock*>(n);

    double all_prog = transition->GetTotalProgress(time);
    double in_prog = transition->GetInProgress(time);
    double out_prog = transition->GetOutProgress(time);

    hash->addData(reinterpret_cast<const char*>(&all_prog), sizeof(double));
    hash->addData(reinterpret_cast<const char*>(&in_prog), sizeof(double));
    hash->addData(reinterpret_cast<const char*>(&out_prog), sizeof(double));
  }

  foreach (NodeParam* param, n->parameters()) {
    // For each input, try to hash its value
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      if (n->IsBlock()) {
        const Block* b = static_cast<const Block*>(n);

        // Ignore some Block attributes when hashing
        if (input == b->media_in_input()
            || input == b->speed_input()
            || input == b->length_input()) {
          continue;
        }
      }

      // Get time adjustment
      // For a single frame, we only care about one of the times
      rational input_time = n->InputTimeAdjustment(input, TimeRange(time, time)).in();

      if (input->IsConnected()) {
        // Traverse down this edge
        HashNodeRecursively(hash, input->get_connected_node(), input_time);
      } else {
        // Grab the value at this time
        QVariant value = input->get_value_at_time(input_time);
        hash->addData(NodeParam::ValueToBytes(input->data_type(), value));
      }

      // We have one exception for FOOTAGE types, since we resolve the footage into a frame in the renderer
      if (input->data_type() == NodeParam::kFootage) {
        StreamPtr stream = ResolveStreamFromInput(input);
        DecoderPtr decoder = ResolveDecoderFromInput(stream);

        if (decoder != nullptr) {
          // Add footage details to hash

          // Footage filename
          hash->addData(stream->footage()->filename().toUtf8());

          // Footage last modified date
          hash->addData(stream->footage()->timestamp().toString().toUtf8());

          // Footage stream
          hash->addData(QString::number(stream->index()).toUtf8());

          if (stream->type() == Stream::kImage || stream->type() == Stream::kVideo) {
            ImageStreamPtr video_stream = std::static_pointer_cast<ImageStream>(stream);

            // Footage timestamp
            hash->addData(QString::number(decoder->GetTimestampFromTime(input_time)).toUtf8());

            // Current color config and space
            hash->addData(video_stream->footage()->project()->ocio_config().toUtf8());
            hash->addData(video_stream->colorspace().toUtf8());

            // Alpha associated setting
            hash->addData(QString::number(video_stream->premultiplied_alpha()).toUtf8());
          }
        }
      }
    }
  }
}

void VideoRenderWorker::SetParameters(const VideoRenderingParams &video_params)
{
  video_params_ = video_params;

  ParametersChangedEvent();
}

bool VideoRenderWorker::InitInternal()
{
  download_buffer_.resize(PixelService::GetBufferSize(video_params().format(), video_params().effective_width(), video_params().effective_height()));
  return true;
}

void VideoRenderWorker::CloseInternal()
{
  download_buffer_.clear();
}

void VideoRenderWorker::Download(NodeDependency dep, QByteArray hash, QVariant texture, QString filename)
{
  PixelFormat::Info format_info = PixelService::GetPixelFormatInfo(video_params().format());

  // Set up OIIO::ImageSpec for compressing cached images on disk
  OIIO::ImageSpec spec(video_params().effective_width(), video_params().effective_height(), kRGBAChannels, format_info.oiio_desc);
  spec.attribute("compression", "dwaa:200");

  TextureToBuffer(texture, download_buffer_);

  std::string working_fn_std = filename.toStdString();

  std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(working_fn_std);

  if (out) {
    out->open(working_fn_std, spec);
    out->write_image(format_info.oiio_desc, download_buffer_.data());
    out->close();
  } else {
    qWarning() << "Failed to open output file:" << filename;
  }
}

NodeValueTable VideoRenderWorker::RenderBlock(const TrackOutput *track, const TimeRange &range)
{
  // A frame can only have one active block so we just validate the in point of the range
  Block* active_block = track->BlockAtTime(range.in());

  NodeValueTable table;

  if (active_block) {
    table = ProcessNode(NodeDependency(active_block,
                                       range));
  }

  return table;
}

ColorProcessorCache *VideoRenderWorker::color_cache()
{
  return &color_cache_;
}
