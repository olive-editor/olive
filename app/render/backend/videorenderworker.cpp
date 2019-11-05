#include "videorenderworker.h"

#include <QThread>

#include "common/define.h"
#include "node/block/block.h"
#include "node/node.h"
#include "render/pixelservice.h"

VideoRenderWorker::VideoRenderWorker(DecoderCache *decoder_cache, QObject *parent) :
  QObject(parent),
  decoder_cache_(decoder_cache),
  started_(false)
{

}

bool VideoRenderWorker::IsAvailable()
{
  return (working_ == 0);
}

void VideoRenderWorker::Close()
{
  CloseInternal();

  started_ = false;
}

const VideoRenderingParams &VideoRenderWorker::video_params()
{
  return video_params_;
}

DecoderCache *VideoRenderWorker::decoder_cache()
{
  return decoder_cache_;
}

void VideoRenderWorker::HashNodeRecursively(QCryptographicHash *hash, Node* n, const rational& time)
{
  // Resolve BlockList
  if (n->IsBlock() && (n = ValidateBlock(n, time)) == nullptr) {
    return;
  }

  // Add this Node's ID
  hash->addData(n->id().toUtf8());

  foreach (NodeParam* param, n->parameters()) {
    // For each input, try to hash its value
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

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

StreamPtr VideoRenderWorker::ResolveStreamFromInput(NodeInput *input)
{
  return input->get_value_at_time(0).value<StreamPtr>();
}

DecoderPtr VideoRenderWorker::ResolveDecoderFromInput(NodeInput *input)
{
  // Access a map of Node inputs and decoder instances and retrieve a frame!
  StreamPtr stream = ResolveStreamFromInput(input);
  DecoderPtr decoder = decoder_cache()->GetDecoder(stream.get());

  if (decoder == nullptr && stream != nullptr) {
    // Init decoder
    decoder = Decoder::CreateFromID(stream->footage()->decoder());
    decoder->set_stream(stream);
    decoder_cache()->AddDecoder(stream.get(), decoder);
  }

  return decoder;
}

Node *VideoRenderWorker::ValidateBlock(Node *n, const rational& time)
{
  if (n->IsBlock()) {
    Block* block = static_cast<Block*>(n);

    while (block != nullptr && block->in() > time) {
      // This Block is too late, find an earlier one
      block = block->previous();
    }

    while (block != nullptr && block->out() <= time) {
      // This block is too early, find a later one
      block = block->next();
    }

    // By this point, we should have the correct Block or nullptr if there's no Block here
    return block;
  }

  return n;
}

void VideoRenderWorker::SetParameters(const VideoRenderingParams &video_params)
{
  video_params_ = video_params;

  ParametersChangedEvent();
}

bool VideoRenderWorker::Init()
{
  if (started_) {
    return true;
  }

  started_ = InitInternal();

  if (started_) {
    download_buffer_.resize(PixelService::GetBufferSize(video_params().format(), video_params().effective_width(), video_params().effective_height()));
  } else {
    Close();
  }

  return started_;
}

bool VideoRenderWorker::IsStarted()
{
  return started_;
}

void VideoRenderWorker::Render(NodeDependency path)
{
  NodeOutput* output = path.node();
  Node* node = output->parent();

  QList<Node*> all_nodes_in_graph;
  all_nodes_in_graph.append(node);
  all_nodes_in_graph.append(node->GetDependencies());

  // Lock all Nodes to prevent UI changes during this render
  foreach (Node* dep, all_nodes_in_graph) {
    dep->LockUserInput();
  }

  // Start traversing graph
  RenderAsSibling(path);

  // Unlock all Nodes so changes can be made again
  foreach (Node* dep, all_nodes_in_graph) {
    dep->UnlockUserInput();
  }

  emit CompletedFrame(path);
}

QList<NodeInput*> VideoRenderWorker::ProcessNodeInputsForTime(Node *n, const TimeRange &time)
{
  QList<NodeInput*> connected_inputs;

  // Now we need to gather information about this Node's inputs
  foreach (NodeParam* param, n->parameters()) {
    // Check if this parameter is an input and if the Node is dependent on it
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      if (input->dependent()) {
        // If we're here, this input is necessary and we need to acquire the value for this Node
        if (input->IsConnected()) {
          // If it's connected to something, we need to retrieve that output at some point
          connected_inputs.append(input);
        } else {
          // If it isn't connected, it'll have the value we need inside it. We just need to store it for the node.
          input->set_stored_value(input->get_value_at_time(n->InputTimeAdjustment(input, time).in()));
        }

        // Special types like FOOTAGE require extra work from us (to decrease node complexity dealing with decoders)
        if (input->data_type() == NodeParam::kFootage) {
          input->set_stored_value(0);

          DecoderPtr decoder = ResolveDecoderFromInput(input);

          // By this point we should definitely have a decoder, and if we don't something's gone terribly wrong
          if (decoder != nullptr) {
            FramePtr frame = decoder->Retrieve(time.in());

            if (frame != nullptr) {
              QVariant value = FrameToTexture(frame);

              input->set_stored_value(value);

              qDebug() << "Placing texture" << value << "into input" << input;
            }
          }
        }
      }
    }
  }

  return connected_inputs;
}

void VideoRenderWorker::RenderAsSibling(NodeDependency dep)
{
  NodeOutput* output = dep.node();
  Node* original_node = output->parent();
  Node* node;
  rational time = dep.in();
  QList<NodeInput*> connected_inputs;
  QVariant value;

  // Set working state
  working_++;

  qDebug() << "Processing" << original_node->id() << original_node;

  original_node->LockProcessing();

  // Firstly we check if this node is a "Block", if it is that means it's part of a linked list of mutually exclusive
  // nodes based on time and we might need to locate which Block to attach to
  if ((node = ValidateBlock(original_node, time)) == nullptr) {
    // ValidateBlock() may have returned nullptr if there was no Block found at this time so no texture to return
    output->cache_value(dep.range(), 0);

    original_node->UnlockProcessing();
    goto end_render;
  }

  if (original_node != node) {
    // Ensure output is the output matching the node as it may have changed
    output = static_cast<NodeOutput*>(node->GetParameterWithID(output->id()));

    // Switch locks
    original_node->UnlockProcessing();
    node->LockProcessing();

    qDebug() << "Deftly switched from" << original_node->id() << original_node << "to" << node->id() << node;
  }

  // Check if the output already has a value for this time
  if (output->has_cached_value(dep.range())) {
    // If so, we don't need to do anything, we can just send this value and exit here
    dep.node()->cache_value(dep.range(), output->get_cached_value(dep.range()));

    qDebug() << "Found a cached value on" << node->id() << output->id() << "at" << dep.range().in() << "-" << dep.range().out();

    node->UnlockProcessing();
    goto end_render;
  }

  // We need to run the Node's code to get the correct value for this time

  connected_inputs = ProcessNodeInputsForTime(node, dep.range());

  // For each connected input, we need to acquire the value from another node
  while (!connected_inputs.isEmpty()) {

    // Remove any inputs from the list that we have valid cached values for already
    for (int i=0;i<connected_inputs.size();i++) {
      NodeInput* input = connected_inputs.at(i);
      NodeOutput* connected_output = input->get_connected_output();
      TimeRange input_time = node->InputTimeAdjustment(input, dep.range());

      if (connected_output->has_cached_value(input_time)) {
        // This output already has this value, no need to process it again
        input->set_stored_value(connected_output->get_cached_value(input_time));
        connected_inputs.removeAt(i);
        i--;
      }
    }

    // For every connected input except the first, we'll request another Node to do it
    int input_for_this_thread = -1;

    for (int i=0;i<connected_inputs.size();i++) {
      NodeInput* input = connected_inputs.at(i);

      // If this node is locked, we assume it's already being processed. Otherwise we need to request a sibling
      if (!input->get_connected_node()->IsProcessingLocked()) {
        if (input_for_this_thread == -1) {
          // Store this later since we can process it on this thread as we wait for other threads
          input_for_this_thread = i;
        } else {
          TimeRange input_time = node->InputTimeAdjustment(input, dep.range());

          emit RequestSibling(NodeDependency(input->get_connected_output(),
                                             input_time));
        }
      }
    }

    if (input_for_this_thread > -1) {
      // In the mean time, this thread can go off to do the first parameter
      NodeInput* input = connected_inputs.at(input_for_this_thread);
      TimeRange input_range = node->InputTimeAdjustment(input, dep.range());
      RenderAsSibling(NodeDependency(input->get_connected_output(),
                                     input_range));
      input->set_stored_value(input->get_connected_output()->get_cached_value(input_range));
      connected_inputs.removeAt(input_for_this_thread);
    } else {
      // Nothing for this thread to do. We'll wait 0.5 sec and check again for other nodes
      // FIXME: It would be nicer if this thread could do other nodes during this time
      QThread::msleep(500);
    }
  }

  // By this point, the node should have all the inputs it needs to render correctly

  // Check if we have a shader for this output
  if (OutputIsShader(output)) {
    // Run code
    value = RunNodeAsShader(output);
  } else {
    // Generate the value as expected
    value = node->Value(output);
  }

  // Place the value into the output
  output->cache_value(dep.range(), value);
  dep.node()->cache_value(dep.range(), value);

  // We're done!
  node->UnlockProcessing();

end_render:
  // End this working state
  working_--;
}



void VideoRenderWorker::Download(NodeDependency dep, QVariant texture, QString filename)
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
    qDebug() << "Saving to" << filename;
    out->open(working_fn_std, spec);
    out->write_image(format_info.oiio_desc, download_buffer_.data());
    out->close();

    emit CompletedDownload(dep);
  } else {
    qWarning() << "Failed to open output file:" << filename;
  }

  working_--;
}
