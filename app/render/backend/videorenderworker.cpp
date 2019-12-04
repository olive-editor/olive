#include "videorenderworker.h"

#include "common/define.h"
#include "node/node.h"
#include "render/pixelservice.h"

VideoRenderWorker::VideoRenderWorker(DecoderCache *decoder_cache, VideoRenderFrameCache *frame_cache, QObject *parent) :
  RenderWorker(decoder_cache, parent),
  frame_cache_(frame_cache)
{

}

const VideoRenderingParams &VideoRenderWorker::video_params()
{
  return video_params_;
}

void VideoRenderWorker::RenderInternal(const NodeDependency& path)
{
  // Get hash of node graph
  // We use SHA-1 for speed (benchmarks show it's the fastest hash available to us)
  QCryptographicHash hasher(QCryptographicHash::Sha1);
  HashNodeRecursively(&hasher, path.node()->parentNode(), path.in());
  QByteArray hash = hasher.result();

  if (frame_cache_->HasHash(hash)) {
    // We've already cached this hash, no need to continue
    emit HashAlreadyExists(path, hash);
  } else if (frame_cache_->TryCache(hash)) {
    // This hash is available for us to cache, start traversing graph
    QVariant value = RenderAsSibling(path);

    emit CompletedFrame(path, hash, value);
  } else {
    // Another thread must be caching this already, nothing to be done
    emit HashAlreadyBeingCached();
  }
}

FramePtr VideoRenderWorker::RetrieveFromDecoder(DecoderPtr decoder, const TimeRange &range)
{
  return decoder->RetrieveVideo(range.in());
}

void VideoRenderWorker::HashNodeRecursively(QCryptographicHash *hash, Node* n, const rational& time)
{
  // Resolve BlockList
  if (n->IsBlock()) {
    n = ValidateBlock(static_cast<Block*>(n), time);
  }

  // Add this Node's ID
  hash->addData(n->id().toUtf8());

  foreach (NodeParam* param, n->parameters()) {
    // For each input, try to hash its value
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      if (input->dependent()) {
        // Get time adjustment
        TimeRange range = n->InputTimeAdjustment(input, TimeRange(time, time));

        // For a single frame, we only care about one of the times
        rational input_time = range.in();

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
          DecoderPtr decoder = ResolveDecoderFromInput(input);

          if (decoder != nullptr) {
            // Add footage details to hash

            // Footage filename
            hash->addData(stream->footage()->filename().toUtf8());

            // Footage last modified date
            hash->addData(stream->footage()->timestamp().toString().toUtf8());

            // Footage stream
            hash->addData(QString::number(stream->index()).toUtf8());

            // Footage timestamp
            hash->addData(QString::number(decoder->GetTimestampFromTime(time)).toUtf8());

            // FIXME: Add colorspace and alpha assoc
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

QVariant VideoRenderWorker::RenderAsSibling(NodeDependency dep)
{
  NodeOutput* output = dep.node();
  Node* original_node = output->parentNode();
  Node* node;
  rational time = dep.in();
  QVariant value;

  // Set working state
  working_++;

  //qDebug() << "Processing" << original_node->id() << original_node;

  // Firstly we check if this node is a "Block", if it is that means it's part of a linked list of mutually exclusive
  // nodes based on time and we might need to locate which Block to attach to
  if (original_node->IsBlock()) {
    node = ValidateBlock(static_cast<Block*>(original_node), time);

    if (original_node != node) {
      // Ensure output is the output matching the node as it may have changed
      output = static_cast<NodeOutput*>(node->GetParameterWithID(output->id()));
    }
  } else {
    node = original_node;
  }

  value = ProcessNodeNormally(NodeDependency(output, dep.range()));

  // We're done!

  // End this working state
  working_--;

  return value;
}

void VideoRenderWorker::Download(NodeDependency dep, QByteArray hash, QVariant texture, QString filename)
{
  working_++;

  PixelFormatInfo format_info = PixelService::GetPixelFormatInfo(video_params().format());

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

    emit CompletedDownload(dep, hash);
  } else {
    qWarning() << "Failed to open output file:" << filename;
  }

  working_--;
}
