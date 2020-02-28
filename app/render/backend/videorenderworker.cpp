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

VideoRenderWorker::VideoRenderWorker(VideoRenderFrameCache *frame_cache, DecoderCache* decoder_cache, QObject *parent) :
  RenderWorker(decoder_cache, parent),
  frame_cache_(frame_cache),
  operating_mode_(kHashRenderCache)
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

    HashNodeRecursively(&hasher, path.node(), path.in());
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
    if (!texture.isNull()) {
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

        if (stream) {
          DecoderPtr decoder = ResolveDecoderFromInput(stream);

          if (decoder) {

            // Add footage details to hash

            // Footage filename
            hash->addData(stream->footage()->filename().toUtf8());

            // Footage last modified date
            hash->addData(stream->footage()->timestamp().toString().toUtf8());

            // Footage stream
            hash->addData(QString::number(stream->index()).toUtf8());

            if (stream->type() == Stream::kImage || stream->type() == Stream::kVideo) {
              ImageStreamPtr image_stream = std::static_pointer_cast<ImageStream>(stream);

              // Current color config and space
              hash->addData(image_stream->footage()->project()->ocio_config().toUtf8());
              hash->addData(image_stream->colorspace().toUtf8());

              // Alpha associated setting
              hash->addData(QString::number(image_stream->premultiplied_alpha()).toUtf8());
            }

            // Footage timestamp
            if (stream->type() == Stream::kVideo) {
              hash->addData(QStringLiteral("%1/%2").arg(QString::number(input_time.numerator()),
                                                        QString::number(input_time.denominator())).toUtf8());
              /*Decoder::RetrieveState state = decoder->GetRetrieveState(input_time);

              if (state == Decoder::kReady) {
                VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream);

                int64_t timestamp_here = video_stream->get_closest_timestamp_in_frame_index(input_time);

                hash->addData(QString::number(timestamp_here).toUtf8());
              } else {
                ReportUnavailableFootage(stream, state, input_time);
              }*/
            }
          }
        }
      }
    }
  }
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

bool VideoRenderWorker::InitInternal()
{
  ResizeDownloadBuffer();
  return true;
}

void VideoRenderWorker::CloseInternal()
{
  download_buffer_.clear();
}

void VideoRenderWorker::Download(const rational& time, QVariant texture, QString filename)
{
  if (operating_mode_ & kDownloadOnly) {

    TextureToBuffer(texture, download_buffer_.data());

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
    frame->set_width(video_params().width());
    frame->set_height(video_params().height());
    frame->set_format(video_params().format());
    frame->allocate();

    TextureToBuffer(texture, frame->data());

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
