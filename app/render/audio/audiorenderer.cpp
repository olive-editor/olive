/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "audiorenderer.h"

#include <OpenImageIO/imageio.h>
#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QtMath>

#include "common/filefunctions.h"
#include "render/gl/functions.h"
#include "render/gl/shadergenerators.h"
#include "render/pixelservice.h"

AudioRendererProcessor::AudioRendererProcessor(QObject *parent) :
  QObject(parent),
  started_(false),
  caching_(false),
  starting_(false),
  viewer_node_(nullptr)
{
  // FIXME: Cache name should actually be the name of the sequence
  SetCacheName("Test");
}

AudioRendererProcessor::~AudioRendererProcessor()
{
  Stop();
}

void AudioRendererProcessor::SetCacheName(const QString &s)
{
  cache_name_ = s;
  cache_time_ = QDateTime::currentMSecsSinceEpoch();

  GenerateCacheIDInternal();
}

void AudioRendererProcessor::InvalidateCache(const rational &start_range, const rational &end_range)
{
  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(viewer_node_->Length(), end_range);

  qDebug() << "Cache invalidated between"
           << start_range_adj.toDouble()
           << "and"
           << end_range_adj.toDouble();

  bool append = true;

  for (int i=0;i<cache_queue_.size();i++) {
    const TimeRange& const_range = cache_queue_.at(i);

    if (start_range_adj >= const_range.in()
        && start_range_adj <= const_range.out()) {
      append = false;
      if (const_range.out() < end_range_adj) {
        // Same in point but longer, extend
        cache_queue_[i].set_out(end_range_adj);
      }
      break;
    } else if (end_range_adj <= const_range.out()
               && end_range_adj >= const_range.in()) {
      append = false;
      if (const_range.in() > start_range_adj) {
        // Same out point but longer, extend
        cache_queue_[i].set_in(start_range_adj);
      }
      break;
    }
  }

  if (append) {
    cache_queue_.append(TimeRange(start_range_adj, end_range_adj));
  }

  CacheNext();
}

void AudioRendererProcessor::SetParameters(const AudioRenderingParams& params)
{
  // Since we're changing parameters, all the existing threads are invalid and must be removed. They will start again
  // next time this Node has to process anything.
  Stop();

  // Set new parameters
  params_ = params;

  // Regenerate the cache ID
  GenerateCacheIDInternal();
}

void AudioRendererProcessor::Start()
{
  if (started_) {
    return;
  }

  QOpenGLContext* ctx = QOpenGLContext::currentContext();

  int background_thread_count = QThread::idealThreadCount();

  // Some OpenGL implementations (notably wgl) require the context not to be current before sharing
  QSurface* old_surface = ctx->surface();
  ctx->doneCurrent();

  threads_.resize(background_thread_count);

  for (int i=0;i<threads_.size();i++) {
    threads_[i] = std::make_shared<AudioRendererProcessThread>(this, params_);
    threads_[i]->StartThread(QThread::LowPriority);

    // Ensure this connection is "Queued" so that it always runs in this object's threaded rather than any of the
    // other threads
    connect(threads_.at(i).get(),
            SIGNAL(RequestSibling(NodeDependency)),
            this,
            SLOT(ThreadRequestSibling(NodeDependency)),
            Qt::QueuedConnection);
  }

  // Connect first thread (master thread) to the callback
  connect(threads_.first().get(),
          SIGNAL(CachedFrame(const QByteArray&, const rational&, const rational&)),
          this,
          SLOT(ThreadCallback(const QByteArray&, const rational&, const rational&)),
          Qt::QueuedConnection);

  // Restore context now that thread creation is complete
  ctx->makeCurrent(old_surface);

  started_ = true;
}

void AudioRendererProcessor::Stop()
{
  if (!started_) {
    return;
  }

  started_ = false;

  foreach (AudioRendererProcessThreadPtr process_thread, threads_) {
    process_thread->Cancel();
  }
  threads_.clear();
}

void AudioRendererProcessor::GenerateCacheIDInternal()
{
  if (cache_name_.isEmpty() || !params_.is_valid()) {
    return;
  }

  // Generate an ID that is more or less guaranteed to be unique to this Sequence
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(cache_name_.toUtf8());
  hash.addData(QString::number(cache_time_).toUtf8());
  hash.addData(QString::number(params_.sample_rate()).toUtf8());
  hash.addData(QString::number(params_.channel_layout()).toUtf8());
  hash.addData(QString::number(params_.format()).toUtf8());

  QByteArray bytes = hash.result();
  cache_id_ = bytes.toHex();
}

void AudioRendererProcessor::CacheNext()
{
  if (cache_queue_.isEmpty() || viewer_node_ == nullptr || caching_) {
    return;
  }

  // Make sure cache has started
  Start();

  TimeRange cache_frame = cache_queue_.takeFirst();

  qDebug() << "Caching" << cache_frame.in().toDouble() << "-" << cache_frame.out().toDouble();

  threads_.first()->Queue(NodeDependency(viewer_node_->texture_input()->get_connected_output(), cache_frame), true, false);

  caching_ = true;
}

QString AudioRendererProcessor::CachePathName(const QByteArray &hash)
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id_);
  this_cache_dir.mkpath(".");

  QString filename = QString("%1.pcm").arg(QString(hash.toHex()));

  return this_cache_dir.filePath(filename);
}

void AudioRendererProcessor::ThreadCallback(const QByteArray& samples, const rational& in, const rational& out)
{
  // Threads are all done now, time to proceed
  caching_ = false;

  int start_offset = params_.time_to_bytes(in);
  int end_offset = params_.time_to_bytes(out);

  // Ensure sample cache is at least large enough for this
  if (sample_cache_.size() < end_offset) {
    sample_cache_.resize(end_offset);
  }

  sample_cache_.replace(start_offset, samples.size(), samples);

  CacheNext();
}

void AudioRendererProcessor::ThreadRequestSibling(NodeDependency dep)
{
  // Try to queue another thread to run this dep in advance
  for (int i=1;i<threads_.size();i++) {
    if (threads_.at(i)->Queue(dep, false, true)) {
      return;
    }
  }
}

AudioRendererThreadBase* AudioRendererProcessor::CurrentThread()
{
  return dynamic_cast<AudioRendererThreadBase*>(QThread::currentThread());
}

AudioParams *AudioRendererProcessor::CurrentInstance()
{
  AudioRendererThreadBase* thread = CurrentThread();

  if (thread != nullptr) {
    return thread->params();
  }

  return nullptr;
}

QByteArray AudioRendererProcessor::GetCachedSamples(const rational &in, const rational &out)
{
  if (viewer_node_ == nullptr || in == out) {
    // Nothing is connected - nothing to show or render
    return nullptr;
  }

  if (!params_.is_valid()) {
    qWarning() << "Invalid parameters";
    return nullptr;
  }

  if (cache_id_.isEmpty()) {
    qWarning() << "No cache ID";
    return nullptr;
  }

  if (out < in || in < 0 || out < 0) {
    qWarning() << "Invalid time requested";
    return nullptr;
  }

  int start_offset = qMin(params_.time_to_bytes(in), sample_cache_.size());
  int end_offset = qMin(params_.time_to_bytes(out), sample_cache_.size());
  int length = end_offset - start_offset;

  if (length == 0) {
    return nullptr;
  }

  return sample_cache_.mid(start_offset, length);
}

void AudioRendererProcessor::SetViewerNode(ViewerOutput *viewer)
{
  if (viewer_node_ != nullptr) {
    disconnect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  }

  viewer_node_ = viewer;

  if (viewer_node_ != nullptr) {
    connect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));

    // FIXME: Hardcoded format and mode
    AudioRenderingParams(viewer_node_->audio_params(), olive::SAMPLE_FMT_FLT);
  }
}
