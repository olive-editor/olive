#include "renderbackend.h"

#include <QDateTime>
#include <QThread>

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  started_(false),
  viewer_node_(nullptr)
{

}

RenderBackend::~RenderBackend()
{
}

bool RenderBackend::Init()
{
  if (started_) {
    return true;
  }

  threads_.resize(QThread::idealThreadCount());

  for (int i=0;i<threads_.size();i++) {
    QThread* thread = new QThread(this);
    threads_.replace(i, thread);

    // We use low priority to keep the app responsive at all times (GUI thread should always prioritize over this one)
    thread->start(QThread::LowPriority);
  }

  started_ = InitInternal();

  if (!started_) {
    Close();
  }

  return started_;
}

void RenderBackend::Close()
{
  if (!started_) {
    return;
  }

  started_ = false;

  CloseInternal();

  decoder_cache_.Clear();

  foreach (QThread* thread, threads_) {
    thread->quit();
    thread->wait(); // FIXME: Maximum time in case a thread is stuck?
  }
  threads_.clear();
}

const QString &RenderBackend::GetError() const
{
  return error_;
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ != nullptr) {
    disconnect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));

    Decompile();
  }

  viewer_node_ = viewer_node;

  if (viewer_node_ != nullptr) {
    connect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  }

  ViewerNodeChangedEvent(viewer_node_);
}

void RenderBackend::SetCacheName(const QString &s)
{
  cache_name_ = s;
  cache_time_ = QDateTime::currentMSecsSinceEpoch();

  RegenerateCacheID();
}

bool RenderBackend::IsInitiated()
{
  return started_;
}

bool RenderBackend::Compile()
{
  if (compiled_) {
    return true;
  }

  compiled_ = CompileInternal();

  if (!compiled_) {
    Decompile();
  }

  return compiled_;
}

void RenderBackend::Decompile()
{
  if (!compiled_) {
    return;
  }

  DecompileInternal();
}

void RenderBackend::RegenerateCacheID()
{
  QCryptographicHash hash(QCryptographicHash::Sha1);

  if (cache_name_.isEmpty()
      || !cache_time_
      || !GenerateCacheIDInternal(hash)) {
    cache_id_.clear();
    return;
  }

  hash.addData(cache_name_.toUtf8());
  hash.addData(QString::number(cache_time_).toUtf8());

  QByteArray bytes = hash.result();
  cache_id_ = bytes.toHex();
}

void RenderBackend::SetError(const QString &error)
{
  error_ = error;
}

void RenderBackend::ViewerNodeChangedEvent(ViewerOutput *node)
{
  Q_UNUSED(node)
}

ViewerOutput *RenderBackend::viewer_node() const
{
  return viewer_node_;
}

const QVector<QThread *> &RenderBackend::threads()
{
  return threads_;
}
