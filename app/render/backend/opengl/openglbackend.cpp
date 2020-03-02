#include "openglbackend.h"

#include <QEventLoop>
#include <QThread>

#include "openglrenderfunctions.h"

OpenGLBackend::OpenGLBackend(QObject *parent) :
  VideoRenderBackend(parent),
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
  proxy_thread->start(QThread::IdlePriority);
  proxy_->moveToThread(proxy_thread);

  if (!proxy_->Init()) {
    proxy_thread->quit();
    proxy_thread->wait();
    proxy_thread->deleteLater();

    proxy_->deleteLater();
    proxy_ = nullptr;
    return false;
  }

  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    // Create one processor object for each thread
    OpenGLWorker* processor = new OpenGLWorker(frame_cache(), decoder_cache());
    processor->SetParameters(params());
    processors_.append(processor);

    connect(processor, &OpenGLWorker::RequestFrameToValue, proxy_, &OpenGLProxy::FrameToValue, Qt::BlockingQueuedConnection);
    connect(processor, &OpenGLWorker::RequestTextureToBuffer, proxy_, &OpenGLProxy::TextureToBuffer, Qt::BlockingQueuedConnection);
    connect(processor, &OpenGLWorker::RequestRunNodeAccelerated, proxy_, &OpenGLProxy::RunNodeAccelerated, Qt::BlockingQueuedConnection);
  }

  return true;
}

void OpenGLBackend::CloseInternal()
{
  if (proxy_) {
    proxy_->thread()->quit();
    proxy_->thread()->wait();
    proxy_->thread()->deleteLater();

    proxy_->deleteLater();
    proxy_ = nullptr;
  }

  VideoRenderBackend::CloseInternal();
}

bool OpenGLBackend::CompileInternal()
{
  return true;
}

void OpenGLBackend::DecompileInternal()
{
}

void OpenGLBackend::ParamsChangedEvent()
{
  // If we're initiated, we need to recreate the texture. Otherwise this backend isn't active so it doesn't matter.
  if (IsInitiated()) {
    proxy_->SetParameters(params());
  }
}
