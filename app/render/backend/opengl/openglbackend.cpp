#include "openglbackend.h"

#include <QEventLoop>
#include <QThread>

#include "functions.h"

OpenGLBackend::OpenGLBackend(QObject *parent) :
  VideoRenderBackend(parent),
  last_download_thread_(0)
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
    OpenGLWorker* processor = new OpenGLWorker(share_ctx, &shader_cache_, &texture_cache_, frame_cache());
    processor->SetParameters(params());
    processors_.append(processor);
  }

  // Create master texture (the one sent to the viewer)
  master_texture_ = std::make_shared<OpenGLTexture>();
  master_texture_->Create(share_ctx, params().effective_width(), params().effective_height(), params().format());

  return true;
}

void OpenGLBackend::CloseInternal()
{
  master_texture_ = nullptr;
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

  QList<Node*> nodes = viewer_node()->GetDependencies();

  foreach (Node* n, nodes) {
    // Check if we have a shader or not
    if (!shader_cache_.Has(n->id()))  {
      // Since we don't have a shader, compile one now

      // If the node has no code, it mustn't be GPU accelerated
      if (!n->IsAccelerated()) {
        // We enter a null shader so we don't try to compile this again
        shader_cache_.Add(n->id(), nullptr);
      } else {
        // Since we have shader code, compile it now
        OpenGLShaderPtr program;

        QString frag_code = n->CodeFragment();
        QString vert_code = n->CodeVertex();

        if (frag_code.isEmpty()) {
          frag_code = OpenGLShader::CodeDefaultFragment();
        }

        if (vert_code.isEmpty()) {
          vert_code = OpenGLShader::CodeDefaultVertex();
        }

        if (!(program = std::make_shared<OpenGLShader>())) {
          SetError(QStringLiteral("Failed to create OpenGL shader object"));
          return false;
        }

        if (!program->create()) {
          SetError(QStringLiteral("Failed to create OpenGL shader on device"));
          return false;
        }

        if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, frag_code)) {
          SetError(QStringLiteral("Failed to add OpenGL fragment shader code"));
          return false;
        }

        if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vert_code)) {
          SetError(QStringLiteral("Failed to add OpenGL vertex shader code"));
          return false;
        }

        if (!program->link()) {
          SetError(QStringLiteral("Failed to compile OpenGL shader: %1").arg(program->log()));
          return false;
        }

        shader_cache_.Add(n->id(), program);
      }
    }
  }

  return true;
}

void OpenGLBackend::DecompileInternal()
{
  shader_cache_.Clear();
}

bool OpenGLBackend::TimeIsQueued(const TimeRange &time)
{
  return cache_queue_.contains(time);
}

void OpenGLBackend::ThreadCompletedFrame(NodeDependency path, QByteArray hash, NodeValueTable table)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  QVariant value = table.Get(NodeParam::kTexture);

  if (value.isNull()) {
    // No frame received, we set hash to an empty
    frame_cache()->RemoveHash(path.in(), hash);
  } else {
    // Received a texture, let's download it
    QString cache_fn = frame_cache()->CachePathName(hash);

    // Find an available worker to download this texture
    QMetaObject::invokeMethod(processors_.at(last_download_thread_%processors_.size()),
                              "Download",
                              Q_ARG(NodeDependency, path),
                              Q_ARG(QByteArray, hash),
                              Q_ARG(QVariant, value),
                              Q_ARG(QString, cache_fn));

    last_download_thread_++;
  }

  // Check if this frame has changed once again, in which case we may not want to draw it (it'll look jittery to the user)
  if (!TimeIsQueued(TimeRange(path.in(), path.in()))) {
    // FIXME: This texture is part of the texture cache and therefore volatile, we should probably copy it here instead
    OpenGLTexturePtr tex = nullptr;

    if (!value.isNull()) {
      tex = value.value<OpenGLTextureCache::ReferencePtr>()->texture();
    }

    emit CachedFrameReady(path.in(), QVariant::fromValue(tex));
  }

  // Queue up a new frame for this worker
  CacheNext();
}

void OpenGLBackend::ThreadCompletedDownload(NodeDependency dep, QByteArray hash)
{
  frame_cache()->SetHash(dep.in(), hash);

  // Emit for each frame that has this hash (some may have been added in ThreadSkippedFrame)
  QList<rational> times_with_this_hash = frame_cache()->TimesWithHash(hash);
  foreach (const rational& t, times_with_this_hash) {
    emit CachedTimeReady(t);
  }
}

void OpenGLBackend::ThreadSkippedFrame(NodeDependency dep, QByteArray hash)
{
  frame_cache()->SetHash(dep.in(), hash);
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  // Queue up a new frame for this worker
  CacheNext();
}

void OpenGLBackend::ThreadHashAlreadyExists(NodeDependency dep, QByteArray hash)
{
  ThreadCompletedDownload(dep, hash);
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  // Queue up a new frame for this worker
  CacheNext();
}
