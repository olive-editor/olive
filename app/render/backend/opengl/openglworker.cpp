#include "openglworker.h"

#include <QThread>

#include "node/block/block.h"
#include "node/node.h"

OpenGLWorker::OpenGLWorker(QOpenGLContext *share_ctx, OpenGLShaderCache *shader_cache, DecoderCache *decoder_cache, QObject *parent) :
  QObject(parent),
  share_ctx_(share_ctx),
  ctx_(nullptr),
  functions_(nullptr),
  shader_cache_(shader_cache),
  decoder_cache_(decoder_cache)
{
  surface_.create();
}

OpenGLWorker::~OpenGLWorker()
{
  surface_.destroy();
}

bool OpenGLWorker::IsStarted()
{
  return ctx_ != nullptr;
}

void OpenGLWorker::SetParameters(const VideoRenderingParams &video_params)
{
  video_params_ = video_params;
}

void OpenGLWorker::Init()
{
  // Create context object
  ctx_ = new QOpenGLContext();

  // Set share context
  ctx_->setShareContext(share_ctx_);

  // Create OpenGL context (automatically destroys any existing if there is one)
  if (!ctx_->create()) {
    qWarning() << "Failed to create OpenGL context in thread" << thread();
    Close();
    return;
  }

  ctx_->moveToThread(this->thread());

  qDebug() << "Processor initialized in thread" << thread() << "- context is in" << ctx_->thread();

  // The rest of the initialization needs to occur in the other thread, so we signal for it to start
  QMetaObject::invokeMethod(this, "FinishInit", Qt::QueuedConnection);
}

void OpenGLWorker::Close()
{
  buffer_.Destroy();

  functions_ = nullptr;
  delete ctx_;
}

void OpenGLWorker::Render(const NodeDependency &path)
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

  // Start OpenGL flushing now while we do clean up work on the CPU
  functions_->glFlush();

  // Unlock all Nodes so changes can be made again
  foreach (Node* dep, all_nodes_in_graph) {
    dep->UnlockUserInput();
  }

  // Now we need the texture done so we call glFinish()
  functions_->glFinish();
}

void OpenGLWorker::UpdateViewportFromParams()
{
  if (functions_ != nullptr && video_params_.is_valid()) {
    functions_->glViewport(0, 0, video_params_.effective_width(), video_params_.effective_height());
  }
}

Node *OpenGLWorker::ValidateBlock(Node *n, const rational& time)
{
  if (n->IsBlock()) {
    Block* block = static_cast<Block*>(n);

    while (block->in() > time && block != nullptr) {
      // This Block is too late, find an earlier one
      block = block->previous();
    }

    while (block->out() <= time && block != nullptr) {
      // This block is too early, find a later one
      block = block->next();
    }

    // By this point, we should have the correct Block or nullptr if there's no Block here
    return block;
  }

  return n;
}

QList<NodeInput*> OpenGLWorker::ProcessNodeInputsForTime(Node *n, const TimeRange &time)
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

          // Access a map of Node inputs and decoder instances and retrieve a frame!
          StreamPtr stream = input->get_value_at_time(0).value<StreamPtr>();
          DecoderPtr decoder = decoder_cache_->GetDecoder(stream.get());

          if (decoder == nullptr && stream != nullptr) {
            // Init decoder
            decoder = Decoder::CreateFromID(stream->footage()->decoder());
            decoder->set_stream(stream);
            decoder_cache_->AddDecoder(stream.get(), decoder);
          }

          // By this point we should definitely have a decoder, and if we don't something's gone terribly wrong
          if (decoder != nullptr) {
            FramePtr frame = decoder->Retrieve(time.in());

            if (frame != nullptr) {
              OpenGLTexturePtr footage_tex = std::make_shared<OpenGLTexture>();
              footage_tex->Create(ctx_, frame, OpenGLTexture::kDoubleBuffer);

              // FIXME: Alpha association and color management

              input->set_stored_value(QVariant::fromValue(footage_tex));
            }
          }
        }
      }
    }
  }

  return connected_inputs;
}

void OpenGLWorker::RunNodeAsShader(Node* node, OpenGLShaderPtr shader)
{
  shader->bind();

  unsigned int input_texture_count = 0;

  foreach (NodeParam* param, node->parameters()) {
    if (param->type() == NodeParam::kInput) {
      // See if the shader has takes this parameter as an input
      int variable_location = shader->uniformLocation(param->id());

      if (variable_location > -1) {
        // This variable is used in the shader, let's set it to our value

        NodeInput* input = static_cast<NodeInput*>(param);
        switch (input->data_type()) {
        case NodeInput::kInt:
          shader->setUniformValue(variable_location, input->value().toInt());
          break;
        case NodeInput::kFloat:
          shader->setUniformValue(variable_location, input->value().toFloat());
          break;
        case NodeInput::kVec2:
          shader->setUniformValue(variable_location, input->value().value<QVector2D>());
          break;
        case NodeInput::kVec3:
          shader->setUniformValue(variable_location, input->value().value<QVector3D>());
          break;
        case NodeInput::kVec4:
          shader->setUniformValue(variable_location, input->value().value<QVector4D>());
          break;
        case NodeInput::kMatrix:
          shader->setUniformValue(variable_location, input->value().value<QMatrix4x4>());
          break;
        case NodeInput::kColor:
          shader->setUniformValue(variable_location, input->value().value<QColor>());
          break;
        case NodeInput::kBoolean:
          shader->setUniformValue(variable_location, input->value().toBool());
          break;
        case NodeInput::kTexture:
        case NodeInput::kFootage:
        {
          OpenGLTexturePtr texture = input->value().value<OpenGLTexturePtr>();
          functions_->glActiveTexture(GL_TEXTURE0 + input_texture_count);
          functions_->glBindTexture(GL_TEXTURE_2D, texture->texture());

          // Set value to bound texture
          shader->setUniformValue(variable_location, input_texture_count);

          input_texture_count++;
          break;
        }
        case NodeInput::kAny:
        case NodeInput::kSamples:
        case NodeInput::kTrack:
        case NodeInput::kString:
        case NodeInput::kRational:
        case NodeInput::kBlock:
        case NodeInput::kFont:
        case NodeInput::kFile:
        case NodeInput::kNone:
          break;
        }
      }
    }
  }

  // Attach texture to framebuffer
  // Bind framebuffer
  // Release framebuffer
  // Detach texture

  // Release any textures we bound before
  while (input_texture_count > 0) {
    input_texture_count--;

    // Release texture here
    functions_->glActiveTexture(GL_TEXTURE0 + input_texture_count);
    functions_->glBindTexture(GL_TEXTURE_2D, 0);
  }

  shader->release();
}

void OpenGLWorker::FinishInit()
{
  // Make context current on that surface
  if (!ctx_->makeCurrent(&surface_)) {
    qWarning() << "Failed to makeCurrent() on offscreen surface in thread" << thread();
    return;
  }

  // Store OpenGL functions instance
  functions_ = ctx_->functions();

  // Set up OpenGL parameters as necessary
  functions_->glEnable(GL_BLEND);
  UpdateViewportFromParams();

  buffer_.Create(ctx_);

  qDebug() << "Context in" << ctx_->thread() << "successfully finished";
}

void OpenGLWorker::RenderAsSibling(const NodeDependency &dep)
{
  NodeOutput* output = dep.node();
  Node* node = output->parent();
  rational time = dep.in();

  node->LockProcessing();

  // Firstly we check if this node is a "Block", if it is that means it's part of a linked list of mutually exclusive
  // nodes based on time and we might need to locate which Block to attach to
  if ((node = ValidateBlock(node, time)) == nullptr) {
    // ValidateBlock() may have returned nullptr if there was no Block found at this time so no texture to return
    dep.node()->cache_value(dep.range(), 0);
    node->UnlockProcessing();
    return;
  }

  // Ensure output is the output matching the node as it may have changed
  output = static_cast<NodeOutput*>(node->GetParameterWithID(output->id()));

  // Check if the output already has a value for this time
  if (output->has_cached_value(dep.range())) {
    // If so, we don't need to do anything, we can just send this value and exit here
    dep.node()->cache_value(dep.range(), output->get_cached_value(dep.range()));
    node->UnlockProcessing();
    return;
  }

  // We need to run the Node's code to get the correct value for this time

  QList<NodeInput*> connected_inputs = ProcessNodeInputsForTime(node, dep.range());

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
  OpenGLShaderPtr shader = shader_cache_->GetShader(output);

  if (shader != nullptr) {
    // Run code
    RunNodeAsShader(node, shader);
  } else {
    // Generate the value as expected
    QVariant value = node->Value(output);

    // Place the value into the output
    output->cache_value(dep.range(), value);
  }

  // We're done!

  node->UnlockProcessing();
}
