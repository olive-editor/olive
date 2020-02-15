#include "openglbackend.h"

#include <QEventLoop>
#include <QThread>

#include "openglrenderfunctions.h"

OpenGLBackend::OpenGLBackend(QObject *parent) :
  VideoRenderBackend(parent),
  master_texture_(nullptr),
  proxy_(nullptr)
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

  proxy_ = new OpenGLProxy();
  proxy_->SetParameters(params());
  QThread* proxy_thread = new QThread();
  proxy_thread->start(QThread::LowPriority);
  proxy_->moveToThread(proxy_thread);

  if (!proxy_->Init()) {
    proxy_thread->quit();
    proxy_thread->wait();
    delete proxy_thread;
    delete proxy_;
    return false;
  }

  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    // Create one processor object for each thread
    OpenGLWorker* processor = new OpenGLWorker(frame_cache());
    processor->SetParameters(params());
    processors_.append(processor);

    connect(processor, &OpenGLWorker::RequestFrameToValue, proxy_, &OpenGLProxy::FrameToValue, Qt::BlockingQueuedConnection);
    connect(processor, &OpenGLWorker::RequestTextureToBuffer, proxy_, &OpenGLProxy::TextureToBuffer, Qt::BlockingQueuedConnection);
    connect(processor, &OpenGLWorker::RequestRunNodeAccelerated, proxy_, &OpenGLProxy::RunNodeAccelerated, Qt::BlockingQueuedConnection);
  }

  // Create master texture (the one sent to the viewer)
  master_texture_ = std::make_shared<OpenGLTexture>();
  master_texture_->Create(QOpenGLContext::currentContext(),
                          params().effective_width(),
                          params().effective_height(),
                          params().format());

  // Create copy buffer/pipeline
  copy_buffer_.Create(QOpenGLContext::currentContext());
  copy_pipeline_ = OpenGLShader::CreateDefault();

  return true;
}

void OpenGLBackend::CloseInternal()
{
  if (proxy_) {
    delete proxy_;
    proxy_ = nullptr;
  }

  copy_buffer_.Destroy();
  copy_pipeline_ = nullptr;
  master_texture_ = nullptr;

  VideoRenderBackend::CloseInternal();
}

OpenGLTexturePtr OpenGLBackend::GetCachedFrameAsTexture(const rational &time)
{
  const char* cached_frame = GetCachedFrame(time);

  if (cached_frame) {
    master_texture_->Upload(cached_frame);
    return master_texture_;
  }

  return nullptr;
}

bool OpenGLBackend::CompileInternal()
{
  return true;
}

void OpenGLBackend::DecompileInternal()
{
}

void OpenGLBackend::EmitCachedFrameReady(const rational &time, const QVariant &value, qint64 job_time)
{
  OpenGLTextureCache::ReferencePtr ref = value.value<OpenGLTextureCache::ReferencePtr>();
  OpenGLTexturePtr tex;

  if (ref && ref->texture()) {
    tex = CopyTexture(ref->texture());
  } else {
    tex = nullptr;
  }

  emit CachedFrameReady(time, QVariant::fromValue(tex), job_time);
}

void OpenGLBackend::ParamsChangedEvent()
{
  // If we're initiated, we need to recreate the texture. Otherwise this backend isn't active so it doesn't matter.
  if (IsInitiated()) {
    master_texture_->Destroy();
    master_texture_->Create(QOpenGLContext::currentContext(),
                            params().effective_width(),
                            params().effective_height(),
                            params().format());

    proxy_->SetParameters(params());
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
