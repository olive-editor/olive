#include "openglbackend.h"

#include <QEventLoop>
#include <QThread>

#include "functions.h"

OpenGLBackend::OpenGLBackend(QObject *parent) :
  VideoRenderBackend(parent)
{
}

OpenGLBackend::~OpenGLBackend()
{
  Close();
}

bool OpenGLBackend::InitInternal()
{
  if (!VideoRenderBackend::InitInternal()) {
    return false;
  }

  QOpenGLContext* share_ctx = QOpenGLContext::currentContext();

  if (share_ctx == nullptr) {
    qCritical() << "No active OpenGL context to connect to";
    return false;
  }

  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    // Create one processor object for each thread
    OpenGLWorker* processor = new OpenGLWorker(share_ctx, &shader_cache_, decoder_cache(), frame_cache());
    processor->SetParameters(params());
    processors_.append(processor);
  }

  // Create master texture (the one sent to the viewer)
  master_texture_ = std::make_shared<OpenGLTexture>();
  master_texture_->Create(share_ctx, params().effective_width(), params().effective_height(), params().format());

  /*
  // Create internal FBO for copying textures
  copy_buffer_.Create(share_ctx);
  copy_buffer_.Attach(master_texture_);
  copy_pipeline_ = OpenGLShader::CreateDefault();
  */

  return true;
}

void OpenGLBackend::CloseInternal()
{
  Decompile();

  //copy_buffer_.Destroy();
  master_texture_ = nullptr;
  //copy_pipeline_ = nullptr;
}

OpenGLTexturePtr OpenGLBackend::GetCachedFrameAsTexture(const rational &time)
{
  const char* cached_frame = GetCachedFrame(time);

  if (cached_frame != nullptr) {
    master_texture_->Upload(cached_frame);

    return master_texture_;
  }

  return nullptr;
}

bool OpenGLBackend::CompileInternal()
{
  if (viewer_node() == nullptr || !viewer_node()->texture_input()->IsConnected()) {
    // Nothing to be done, nothing to compile
    return true;
  }

  // Traverse node graph compiling where necessary
  bool ret = TraverseCompiling(viewer_node());

  if (ret) {
    //qDebug() << "Compiled successfully!";
    compiled_ = true;
  } else {
    qDebug() << "Compile failed:" << GetError();
    Decompile();
  }

  return ret;
}

void OpenGLBackend::DecompileInternal()
{
  shader_cache_.Clear();
  compiled_ = false;
}

bool OpenGLBackend::TraverseCompiling(Node *n)
{
  foreach (NodeParam* param, n->parameters()) {
    if (param->type() == NodeParam::kInput && param->IsConnected()) {
      NodeOutput* connected_output = static_cast<NodeInput*>(param)->get_connected_output();

      // Check if we have a shader or not
      if (shader_cache_.GetShader(connected_output) == nullptr)  {
        // Since we don't have a shader, compile one now
        QString node_code = connected_output->parent()->Code(connected_output);

        // If the node has no code, it mustn't be GPU accelerated
        if (!node_code.isEmpty()) {
          // Since we have shader code, compile it now
          OpenGLShaderPtr program;

          if (!(program = std::make_shared<OpenGLShader>())) {
            SetError(QStringLiteral("Failed to create OpenGL shader object"));
            return false;
          }

          if (!program->create()) {
            SetError(QStringLiteral("Failed to create OpenGL shader on device"));
            return false;
          }

          if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, node_code)) {
            SetError(QStringLiteral("Failed to add OpenGL fragment shader code"));
            return false;
          }

          if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, OpenGLShader::CodeDefaultVertex())) {
            SetError(QStringLiteral("Failed to add OpenGL vertex shader code"));
            return false;
          }

          if (!program->link()) {
            SetError(QStringLiteral("Failed to compile OpenGL shader: %1").arg(program->log()));
            return false;
          }

          shader_cache_.AddShader(connected_output, program);

          //qDebug() << "Compiled" <<  connected_output->parent()->id() << "->" << connected_output->id();
        }
      }

      if (!TraverseCompiling(connected_output->parent())) {
        return false;
      }
    }
  }

  return true;
}

bool OpenGLBackend::TimeIsCached(const TimeRange &time)
{
  return cache_queue_.contains(time);
}

void OpenGLBackend::ThreadCompletedFrame(NodeDependency path, QByteArray hash, QVariant value)
{
  caching_ = false;

  OpenGLTexturePtr texture = value.value<OpenGLTexturePtr>();

  if (texture == nullptr) {
    // No frame received, we set hash to an empty
    frame_cache()->RemoveHash(path.in(), hash);
  } else {
    // Received a texture, let's download it
    QString cache_fn = frame_cache()->CachePathName(hash);

    // Find an available worker to download this texture
    foreach (RenderWorker* worker, processors_) {
      // Check if one is available, but worst case if none of them are available, just queue it on the last worker since
      // it's the least likely to get work
      if (worker->IsAvailable() || worker == processors_.last()) {
        QMetaObject::invokeMethod(worker,
                                  "Download",
                                  Q_ARG(NodeDependency, path),
                                  Q_ARG(QByteArray, hash),
                                  Q_ARG(QVariant, QVariant::fromValue(texture)),
                                  Q_ARG(QString, cache_fn));
        break;
      }
    }
  }

  // Set as push texture
  if (!TimeIsCached(TimeRange(path.in(), path.in()))) {
    emit CachedFrameReady(path.in(), value);
  }

  CacheNext();
}

void OpenGLBackend::ThreadCompletedDownload(NodeDependency dep, QByteArray hash)
{
  frame_cache()->SetHash(dep.in(), hash);

  emit CachedTimeReady(dep.in());
}

void OpenGLBackend::ThreadSkippedFrame()
{
  caching_ = false;
  CacheNext();
}

void OpenGLBackend::ThreadHashAlreadyExists(NodeDependency dep, QByteArray hash)
{
  ThreadCompletedDownload(dep, hash);

  ThreadSkippedFrame();
}
