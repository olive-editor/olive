#include "openglbackend.h"

#include <QEventLoop>
#include <QThread>

#include "openglrenderfunctions.h"

OpenGLBackend::OpenGLBackend(QObject *parent) :
  VideoRenderBackend(parent),
  master_texture_(nullptr)
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

  // Create copy buffer/pipeline
  copy_buffer_.Create(share_ctx);
  copy_pipeline_ = OpenGLShader::CreateDefault();

  return true;
}

void OpenGLBackend::CloseInternal()
{
  copy_buffer_.Destroy();
  copy_pipeline_ = nullptr;
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

        QString frag_code = n->AcceleratedCodeFragment();
        QString vert_code = n->AcceleratedCodeVertex();

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

void OpenGLBackend::EmitCachedFrameReady(const QList<rational> &times, const QVariant &value)
{
  OpenGLTextureCache::ReferencePtr ref = value.value<OpenGLTextureCache::ReferencePtr>();
  OpenGLTexturePtr tex;

  if (ref && ref->texture()) {
    tex = CopyTexture(ref->texture());
  } else {
    tex = nullptr;
  }

  foreach (const rational& t, times) {
    emit CachedFrameReady(t, QVariant::fromValue(tex));
  }
}

OpenGLTexturePtr OpenGLBackend::CopyTexture(OpenGLTexturePtr input)
{
  QOpenGLContext* ctx = QOpenGLContext::currentContext();

  OpenGLTexturePtr copy = std::make_shared<OpenGLTexture>();
  copy->Create(ctx, input->width(), input->height(), input->format());

  ctx->functions()->glViewport(0, 0, input->width(), input->height());

  copy_buffer_.Attach(copy);
  copy_buffer_.Bind();
  input->Bind();

  OpenGLRenderFunctions::Blit(copy_pipeline_);

  input->Release();
  copy_buffer_.Release();
  copy_buffer_.Detach();

  return copy;
}
