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
  width_(0),
  height_(0),
  divider_(1),
  caching_(false),
  push_time_(-1),
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
  if (timebase_.isNull()) {
    return;
  }

  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(viewer_node_->Length(), end_range);

  qDebug() << "Cache invalidated between"
           << start_range_adj.toDouble()
           << "and"
           << end_range_adj.toDouble();

  // Snap start_range to timebase
  double start_range_dbl = start_range_adj.toDouble();
  double start_range_numf = start_range_dbl * static_cast<double>(timebase_.denominator());
  int64_t start_range_numround = qFloor(start_range_numf/static_cast<double>(timebase_.numerator())) * timebase_.numerator();
  rational true_start_range(start_range_numround, timebase_.denominator());

  for (rational r=true_start_range;r<=end_range_adj;r+=timebase_) {
    // Try to order the queue from closest to the playhead to furthest
    rational last_time = last_time_requested_;

    rational diff = r - last_time;

    if (diff < 0) {
      // FIXME: Hardcoded number
      // If the number is before the playhead, we still prioritize its closeness but not nearly as much (5:1 in this
      // example)
      diff = qAbs(diff) * 5;
    }

    bool contains = false;
    bool added = false;
    QLinkedList<rational>::iterator insert_iterator;

    for (QLinkedList<rational>::iterator i = cache_queue_.begin();i != cache_queue_.end();i++) {
      rational compare = *i;

      if (!added) {
        rational compare_diff = compare - last_time;

        if (compare_diff > diff) {
          insert_iterator = i;
          added = true;
        }
      }

      if (compare == r) {
        contains = true;
        break;
      }
    }

    if (!contains) {
      if (added) {
        cache_queue_.insert(insert_iterator, r);
      } else {
        cache_queue_.append(r);
      }
    }
  }

  CacheNext();
}

void AudioRendererProcessor::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();
}

void AudioRendererProcessor::SetParameters(const int &width,
                                      const int &height,
                                      const olive::PixelFormat &format,
                                      const olive::RenderMode &mode,
                                      const int& divider)
{
  // Since we're changing parameters, all the existing threads are invalid and must be removed. They will start again
  // next time this Node has to process anything.
  Stop();

  // Set new parameters
  width_ = width;
  height_ = height;
  format_ = format;
  mode_ = mode;

  // divider's default value is 0, so we can assume if it's 0 a divider wasn't specified
  if (divider > 0) {
    divider_ = divider;
  }

  CalculateEffectiveDimensions();

  // Regenerate the cache ID
  GenerateCacheIDInternal();
}

void AudioRendererProcessor::SetDivider(const int &divider)
{
  Q_ASSERT(divider_ > 0);

  Stop();

  divider_ = divider;

  CalculateEffectiveDimensions();

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
    threads_[i] = std::make_shared<AudioRendererProcessThread>(this, ctx, effective_width_, effective_height_, divider_, format_, mode_);
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
          SIGNAL(CachedFrame(RenderTexturePtr, const rational&, const QByteArray&)),
          this,
          SLOT(ThreadCallback(RenderTexturePtr, const rational&, const QByteArray&)),
          Qt::QueuedConnection);
  connect(threads_.first().get(),
          SIGNAL(FrameSkipped(const rational&, const QByteArray&)),
          this,
          SLOT(ThreadSkippedFrame(const rational&, const QByteArray&)),
          Qt::QueuedConnection);

  download_threads_.resize(background_thread_count);

  for (int i=0;i<download_threads_.size();i++) {
    // Create download thread
    download_threads_[i] = std::make_shared<AudioRendererDownloadThread>(ctx, effective_width_, effective_height_, divider_, format_, mode_);
    download_threads_[i]->StartThread(QThread::LowPriority);

    connect(download_threads_[i].get(),
            SIGNAL(Downloaded(const QByteArray&)),
            this,
            SLOT(DownloadThreadComplete(const QByteArray&)),
            Qt::QueuedConnection);
  }

  last_download_thread_ = 0;

  // Restore context now that thread creation is complete
  ctx->makeCurrent(old_surface);

  // Create master texture (the one sent to the viewer)
  master_texture_ = std::make_shared<RenderTexture>();
  master_texture_->Create(ctx, effective_width_, effective_height_, format_);

  // Create internal FBO for copying textures
  copy_buffer_.Create(ctx);
  copy_buffer_.Attach(master_texture_);
  copy_pipeline_ = olive::ShaderGenerator::DefaultPipeline();

  cache_frame_load_buffer_.resize(PixelService::GetBufferSize(format_, effective_width_, effective_height_));

  started_ = true;
}

void AudioRendererProcessor::Stop()
{
  if (!started_) {
    return;
  }

  started_ = false;

  foreach (AudioRendererDownloadThreadPtr download_thread_, download_threads_) {
    download_thread_->Cancel();
  }
  download_threads_.clear();

  foreach (AudioRendererProcessThreadPtr process_thread, threads_) {
    process_thread->Cancel();
  }
  threads_.clear();

  copy_buffer_.Destroy();
  master_texture_ = nullptr;
  copy_pipeline_ = nullptr;

  cache_frame_load_buffer_.clear();
}

void AudioRendererProcessor::GenerateCacheIDInternal()
{
  if (cache_name_.isEmpty() || effective_width_ == 0 || effective_height_ == 0) {
    return;
  }

  // Generate an ID that is more or less guaranteed to be unique to this Sequence
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(cache_name_.toUtf8());
  hash.addData(QString::number(cache_time_).toUtf8());
  hash.addData(QString::number(width_).toUtf8());
  hash.addData(QString::number(height_).toUtf8());
  hash.addData(QString::number(format_).toUtf8());
  hash.addData(QString::number(divider_).toUtf8());

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

  rational cache_frame = cache_queue_.takeFirst();

  qDebug() << "Caching" << cache_frame.toDouble();

  threads_.first()->Queue(NodeDependency(viewer_node_->texture_input()->get_connected_output(), cache_frame), true, false);

  caching_ = true;
}

QString AudioRendererProcessor::CachePathName(const QByteArray &hash)
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id_);
  this_cache_dir.mkpath(".");

  QString filename = QString("%1.exr").arg(QString(hash.toHex()));

  return this_cache_dir.filePath(filename);
}

void AudioRendererProcessor::DeferMap(const rational &time, const QByteArray &hash)
{
  deferred_maps_.append({time, hash});
}

bool AudioRendererProcessor::HasHash(const QByteArray &hash)
{
  return QFileInfo::exists(CachePathName(hash));
}

bool AudioRendererProcessor::IsCaching(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();

  bool is_caching = cache_hash_list_.contains(hash);

  cache_hash_list_mutex_.unlock();

  return is_caching;
}

bool AudioRendererProcessor::TryCache(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();

  bool is_caching = cache_hash_list_.contains(hash);

  if (!is_caching) {
    cache_hash_list_.append(hash);
  }

  cache_hash_list_mutex_.unlock();

  return !is_caching;
}

void AudioRendererProcessor::CalculateEffectiveDimensions()
{
  effective_width_ = width_ / divider_;
  effective_height_ = height_ / divider_;
}

void AudioRendererProcessor::ThreadCallback(RenderTexturePtr texture, const rational& time, const QByteArray& hash)
{
  // Threads are all done now, time to proceed
  caching_ = false;

  DeferMap(time, hash);

  if (texture != nullptr) {
    // We received a texture, time to start downloading it
    QString fn = CachePathName(hash);

    download_threads_[last_download_thread_%download_threads_.size()]->Queue(texture,
                                                                             fn,
                                                                             hash);

    last_download_thread_++;
  } else {
    // There was no texture here, we must update the viewer
    DownloadThreadComplete(hash);
  }

  // If the connected output is using this time, signal it to update
  if (last_time_requested_ == time) {
    copy_buffer_.Bind();
    texture->Bind();

    QOpenGLContext::currentContext()->functions()->glViewport(0, 0, master_texture_->width(), master_texture_->height());

    olive::gl::Blit(copy_pipeline_);

    texture->Release();
    copy_buffer_.Release();

    push_time_ = time;

    emit CachedFrameReady(time);
  }

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

void AudioRendererProcessor::ThreadSkippedFrame(const rational& time, const QByteArray& hash)
{
  caching_ = false;

  DeferMap(time, hash);

  if (!IsCaching(hash)) {
    DownloadThreadComplete(hash);

    // Signal output to update value
    emit CachedFrameReady(time);
  }

  CacheNext();
}

void AudioRendererProcessor::DownloadThreadComplete(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();
  cache_hash_list_.removeAll(hash);
  cache_hash_list_mutex_.unlock();

  for (int i=0;i<deferred_maps_.size();i++) {
    const HashTimeMapping& deferred = deferred_maps_.at(i);

    if (deferred_maps_.at(i).hash == hash) {
      // Insert into hash map
      time_hash_map_.insert(deferred.time, deferred.hash);

      deferred_maps_.removeAt(i);
      i--;
    }
  }
}

AudioRendererThreadBase* AudioRendererProcessor::CurrentThread()
{
  return dynamic_cast<AudioRendererThreadBase*>(QThread::currentThread());
}

RenderInstance *AudioRendererProcessor::CurrentInstance()
{
  AudioRendererThreadBase* thread = CurrentThread();

  if (thread != nullptr) {
    return thread->render_instance();
  }

  return nullptr;
}

RenderTexturePtr AudioRendererProcessor::GetCachedFrame(const rational &time)
{
  last_time_requested_ = time;

  if (push_time_ >= 0) {
    rational temp_push_time = push_time_;
    push_time_ = -1;

    if (time == temp_push_time) {
      return master_texture_;
    }
  }

  if (viewer_node_ == nullptr) {
    // Nothing is connected - nothing to show or render
    return nullptr;
  }

  if (cache_id_.isEmpty()) {
    qWarning() << "RendererProcessor has no cache ID";
    return nullptr;
  }

  if (timebase_.isNull()) {
    qWarning() << "RendererProcessor has no timebase";
    return nullptr;
  }

  // Find frame in map
  if (time_hash_map_.contains(time)) {
    QString fn = CachePathName(time_hash_map_[time]);

    if (QFileInfo::exists(fn)) {
      auto in = OIIO::ImageInput::open(fn.toStdString());

      if (in) {
        in->read_image(PixelService::GetPixelFormatInfo(format_).oiio_desc, cache_frame_load_buffer_.data());

        in->close();

        master_texture_->Upload(cache_frame_load_buffer_.data());

        return master_texture_;
      } else {
        qWarning() << "OIIO Error:" << OIIO::geterror().c_str();
      }
    }
  }

  return nullptr;
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
    SetParameters(viewer_node_->ViewerWidth(),
                  viewer_node_->ViewerHeight(),
                  olive::PIX_FMT_RGBA16F,
                  olive::kOffline);
  }
}
