#include "openglbackend.h"

#include <QThread>

OpenGLBackend::OpenGLBackend(QOpenGLContext *share_ctx) :
  share_ctx_(share_ctx)
{
}

OpenGLBackend::~OpenGLBackend()
{
  Close();
}

bool OpenGLBackend::Init()
{
  threads_.resize(QThread::idealThreadCount());

  // Some OpenGL implementations (notably wgl) require the context not to be current before sharing. We block the main
  // thread here to prevent QOpenGLWidget trying to reclaim the context before we're done
  QSurface* old_surface = share_ctx_->surface();
  share_ctx_->doneCurrent();

  // Initiate one thread per CPU core
  for (int i=0;i<threads_.size();i++) {
    // Instantiate thread
    QThread* thread = new QThread(this);
    threads_.replace(i, thread);

    // Create one processor object for each thread
    OpenGLProcessor* processor = new OpenGLProcessor(share_ctx_, thread);

    // FIXME: Hardcoded values
    processor->SetParameters(VideoRenderingParams(viewer_node()->video_params(), olive::PIX_FMT_RGBA16F, olive::kOffline));

    // Finally, we can move it to its own thread
    processor->moveToThread(thread);
  }

  // We've finished creating shared contexts, we can now restore the context to its previous current state
  share_ctx_->makeCurrent(old_surface);

  return true;
}

void OpenGLBackend::GenerateFrame(const rational &time)
{
  Q_UNUSED(time)
}

void OpenGLBackend::Close()
{
  Decompile();

  // Clear all cores
  for (int i=0;i<threads_.size();i++) {
    delete threads_.at(i);
  }
  threads_.clear();
}

bool OpenGLBackend::Compile()
{
  Decompile();

  if (viewer_node() == nullptr || !viewer_node()->texture_input()->IsConnected()) {
    // Nothing to be done, nothing to compile
    return true;
  }

  // Traverse node graph compiling where necessary
  bool ret = TraverseCompiling(viewer_node());

  if (ret) {
    qDebug() << "Compiled successfully!";
  } else {
    qDebug() << "Compile failed:" << GetError();
  }

  return ret;
}

void OpenGLBackend::Decompile()
{
  foreach (const CompiledNode& info, compiled_nodes_) {
    delete info.program;
  }
  compiled_nodes_.clear();
}

bool OpenGLBackend::TraverseCompiling(Node *n)
{
  foreach (NodeParam* param, n->parameters()) {
    if (param->type() == NodeParam::kInput && param->IsConnected()) {
      NodeOutput* connected_output = static_cast<NodeInput*>(param)->get_connected_output();

      // Generate the ID we'd use for this shader
      QString output_id = GenerateShaderID(connected_output);

      // Check if we have a shader or not
      if (GetShaderFromID(output_id) == nullptr) {
        // Since we don't have a shader, compile one now
        QString node_code = connected_output->parent()->Code(connected_output);

        // If the node has no code, it mustn't be GPU accelerated
        if (!node_code.isEmpty()) {
          // Since we have shader code, compile it now
          CompiledNode compiled_info;
          compiled_info.id = output_id;

          if (!(compiled_info.program = new QOpenGLShaderProgram())) {
            SetError("Failed to create OpenGL shader object");
            return false;
          }

          if (!compiled_info.program->create()) {
            SetError("Failed to create OpenGL shader on device");
            return false;
          }

          if (!compiled_info.program->addShaderFromSourceCode(QOpenGLShader::Fragment, node_code)) {
            SetError("Failed to add OpenGL shader code");
            return false;
          }

          if (compiled_info.program->link()) {
            SetError("Failed to compile OpenGL shader");
            return false;
          }

          compiled_nodes_.append(compiled_info);

          qDebug() << "Compiled" << compiled_info.id;
        }
      }

      if (!TraverseCompiling(connected_output->parent())) {
        return false;
      }
    }
  }

  return true;
}

QOpenGLShaderProgram* OpenGLBackend::GetShaderFromID(const QString &id)
{
  foreach (const CompiledNode& info, compiled_nodes_) {
    if (info.id == id) {
      return info.program;
    }
  }

  return nullptr;
}

QString OpenGLBackend::GenerateShaderID(NodeOutput *output)
{
  // Creates a unique identifier for this specific node and this specific output
  return QString("%1:%2").arg(output->parent()->id(), output->id());
}

OpenGLProcessor::OpenGLProcessor(QOpenGLContext *share_ctx, QObject *parent) :
  QObject(parent),
  share_ctx_(share_ctx),
  ctx_(nullptr),
  functions_(nullptr)
{
  surface_.create();
}

OpenGLProcessor::~OpenGLProcessor()
{
  surface_.destroy();
}

bool OpenGLProcessor::IsStarted()
{
  return ctx_ != nullptr;
}

void OpenGLProcessor::SetParameters(const VideoRenderingParams &video_params)
{
  video_params_ = video_params;
}

void OpenGLProcessor::Init()
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

  // Make context current on that surface
  if (!ctx_->makeCurrent(&surface_)) {
    qWarning() << "Failed to makeCurrent() on offscreen surface in thread" << thread();
    Close();
    return;
  }

  // Store OpenGL functions instance
  functions_ = ctx_->functions();

  // Set up OpenGL parameters as necessary
  functions_->glEnable(GL_BLEND);
  UpdateViewportFromParams();

  buffer_.Create(ctx_);
}

void OpenGLProcessor::Close()
{
  buffer_.Destroy();

  functions_ = nullptr;
  delete ctx_;
}

void OpenGLProcessor::UpdateViewportFromParams()
{
  if (functions_ != nullptr && video_params_.is_valid()) {
    functions_->glViewport(0, 0, video_params_.effective_width(), video_params_.effective_height());
  }
}
