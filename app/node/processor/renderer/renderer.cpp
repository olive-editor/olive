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

#include "renderer.h"

#include <OpenImageIO/imageio.h>
#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QtMath>

#include "common/filefunctions.h"
#include "render/pixelservice.h"

RendererProcessor::RendererProcessor() :
  started_(false),
  width_(0),
  height_(0),
  divider_(1),
  caching_(false)
{
  texture_input_ = new NodeInput("tex_in");
  texture_input_->add_data_input(NodeInput::kTexture);
  AddParameter(texture_input_);

  length_input_ = new NodeInput("length_in");
  length_input_->add_data_input(NodeInput::kRational);
  AddParameter(length_input_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeInput::kTexture);
  AddParameter(texture_output_);
}

QString RendererProcessor::Name()
{
  return tr("Renderer");
}

QString RendererProcessor::Category()
{
  return tr("Processor");
}

QString RendererProcessor::Description()
{
  return tr("A multi-threaded OpenGL hardware-accelerated node compositor.");
}

QString RendererProcessor::id()
{
  return "org.olivevideoeditor.Olive.renderervenus";
}

void RendererProcessor::SetCacheName(const QString &s)
{
  cache_name_ = s;
  cache_time_ = QDateTime::currentMSecsSinceEpoch();

  GenerateCacheIDInternal();
}

QVariant RendererProcessor::Value(NodeOutput* output, const rational& time)
{
  if (output == texture_output_) {
    if (!texture_input_->IsConnected()) {
      // Nothing is connected - nothing to show or render
      return 0;
    }

    if (cache_id_.isEmpty()) {
      qWarning() << "RendererProcessor has no cache ID";
      return 0;
    }

    if (timebase_.isNull()) {
      qWarning() << "RendererProcessor has no timebase";
      return 0;
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

          return QVariant::fromValue(master_texture_);
        }
      }
    }
  }

  return 0;
}

void RendererProcessor::Release()
{
  Stop();
}

void RendererProcessor::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Q_UNUSED(from)

  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(length_input()->get_value(0).value<rational>(), end_range);

  qDebug() << "[RendererProcessor] Cache invalidated between"
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
    rational last_time = texture_output()->LastRequestedTime();

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

    time_hash_map_.remove(r);
  }

  CacheNext();
}

void RendererProcessor::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();
}

void RendererProcessor::SetParameters(const int &width,
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

void RendererProcessor::SetDivider(const int &divider)
{
  Q_ASSERT(divider_ > 0);

  Stop();

  divider_ = divider;

  CalculateEffectiveDimensions();

  // Regenerate the cache ID
  GenerateCacheIDInternal();
}

void RendererProcessor::Start()
{
  if (started_) {
    return;
  }

  QOpenGLContext* ctx = QOpenGLContext::currentContext();

  int background_thread_count = QThread::idealThreadCount();

  threads_.resize(background_thread_count);

  for (int i=0;i<threads_.size();i++) {
    threads_[i] = std::make_shared<RendererProcessThread>(this, ctx, effective_width_, effective_height_, format_, mode_);
    threads_[i]->StartThread(QThread::LowPriority);

    // Ensure this connection is "Queued" so that it always runs in this object's threaded rather than any of the
    // other threads
    connect(threads_[i].get(), SIGNAL(FinishedPath()), this, SLOT(ThreadCallback()), Qt::QueuedConnection);
    connect(threads_[i].get(), SIGNAL(RequestSibling(NodeDependency)), this, SLOT(ThreadRequestSibling(NodeDependency)), Qt::QueuedConnection);
  }

  download_threads_.resize(background_thread_count);

  for (int i=0;i<download_threads_.size();i++) {
    // Create download thread
    download_threads_[i] = std::make_shared<RendererDownloadThread>(ctx, effective_width_, effective_height_, format_, mode_);
    download_threads_[i]->StartThread(QThread::LowPriority);

    connect(download_threads_[i].get(), SIGNAL(Downloaded(const rational&)), this, SLOT(DownloadThreadFinished(const rational&)));
  }

  last_download_thread_ = 0;

  // Create master texture (the one sent to the viewer)
  master_texture_ = std::make_shared<RenderTexture>();
  master_texture_->Create(ctx, effective_width_, effective_height_, format_);

  cache_frame_load_buffer_.resize(PixelService::GetBufferSize(format_, effective_width_, effective_height_));

  started_ = true;
}

void RendererProcessor::Stop()
{
  if (!started_) {
    return;
  }

  started_ = false;

  foreach (RendererDownloadThreadPtr download_thread_, download_threads_) {
    download_thread_->Cancel();
  }
  download_threads_.clear();

  foreach (RendererProcessThreadPtr process_thread, threads_) {
    process_thread->Cancel();
  }
  threads_.clear();

  master_texture_ = nullptr;

  cache_frame_load_buffer_.clear();
}

void RendererProcessor::GenerateCacheIDInternal()
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

void RendererProcessor::CacheNext()
{
  if (cache_queue_.isEmpty() || !texture_input_->IsConnected() || caching_) {
    return;
  }

  // Make sure cache has started
  Start();

  cache_frame_ = cache_queue_.takeFirst();

  qDebug() << "[RendererProcessor] Caching" << cache_frame_.toDouble();

  master_thread_ = threads_.at(0).get();
  master_thread_->Queue(NodeDependency(texture_input_->get_connected_output(), cache_frame_), true);

  caching_ = true;
}

QString RendererProcessor::CachePathName(const QByteArray &hash)
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id_);
  this_cache_dir.mkpath(".");

  QString filename = QString("%1.exr").arg(QString(hash.toHex()));

  return this_cache_dir.filePath(filename);
}

bool RendererProcessor::HasHash(const QByteArray &hash)
{
  return QFileInfo::exists(CachePathName(hash));
}

void RendererProcessor::CalculateEffectiveDimensions()
{
  effective_width_ = width_ / divider_;
  effective_height_ = height_ / divider_;
}

void RendererProcessor::ThreadCallback()
{
  if (sender() == master_thread_) {
    // Threads are all done now, time to proceed
    caching_ = false;

    RenderTexturePtr texture = master_thread_->texture();

    if (texture != nullptr) {
      QString fn = CachePathName(master_thread_->hash());
      download_threads_[last_download_thread_%download_threads_.size()]->Queue(texture, fn, cache_frame_);
      last_download_thread_++;
    }

    time_hash_map_.insert(cache_frame_, master_thread_->hash());

    // We didn't receive a texture to download, but the viewer may still need updating
    if (texture == nullptr) {
      DownloadThreadFinished(cache_frame_);
    }

    CacheNext();
  }
}

void RendererProcessor::ThreadRequestSibling(NodeDependency dep)
{
  // Try to queue another thread to run this dep in advance
  for (int i=0;i<threads_.size();i++) {
    if (threads_.at(i)->Queue(dep, false)) {
      return;
    }
  }
}

void RendererProcessor::DownloadThreadFinished(const rational& time)
{
  // Check if we just downloaded (akak finished caching) the frame we're currently on
  if (texture_output_->IsConnected() && texture_output_->LastRequestedTime() == time) {
    texture_output_->ClearCachedValue();

    Node::InvalidateCache(time, time, nullptr);
  }
}

RendererThreadBase* RendererProcessor::CurrentThread()
{
  return dynamic_cast<RendererThreadBase*>(QThread::currentThread());
}

RenderInstance *RendererProcessor::CurrentInstance()
{
  RendererThreadBase* thread = CurrentThread();

  if (thread != nullptr) {
    return thread->render_instance();
  }

  return nullptr;
}

NodeInput *RendererProcessor::texture_input()
{
  return texture_input_;
}

NodeInput *RendererProcessor::length_input()
{
  return length_input_;
}

NodeOutput *RendererProcessor::texture_output()
{
  return texture_output_;
}
