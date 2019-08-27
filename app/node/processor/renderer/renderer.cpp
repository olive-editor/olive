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

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QtMath>

#include "renderpath.h"
#include "rendererprobe.h"

RendererProcessor::RendererProcessor() :
  started_(false),
  width_(0),
  height_(0),
  caching_(false)
{
  texture_input_ = new NodeInput("tex_in");
  texture_input_->add_data_input(NodeInput::kTexture);
  AddParameter(texture_input_);

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

void RendererProcessor::Process(const rational &time)
{
  Q_UNUSED(time)

  texture_output_->set_value(0);

  if (!texture_input_->IsConnected()) {
    // Nothing is connected - nothing to show or render
    return;
  }

  if (cache_id_.isEmpty()) {
    qWarning() << "RendererProcessor has no cache ID";
    return;
  }

  if (timebase_.isNull()) {
    qWarning() << "RendererProcessor has no timebase";
    return;
  }

  // This Renderer node relies on a disk cache so this Process() function should be quite fast. Either it returns the
  // cached frame or it returns nothing.

  // Perhaps it should lookahead to load textures into VRAM in advance?

  // Should it cache the final result or the result in an 8-bit image or a 16-bit intermediate image?

  /*qDebug() << QString("Requesting %1/%2").arg(QString::number(time.numerator()), QString::number(time.denominator()));

  if (cache_map_.contains(time)) {
    qDebug() << "  We have this frame!";
  } else {
    qDebug() << "  No frame at this address";
    cache_map_.insert(time, true);
  }*/





  // FIXME: Test code only
  //GLuint tex = texture_input_->get_value(time).value<GLuint>();

  //glReadPixels()

  //texture_output_->set_value(tex);
  // End test code

  /*
  // Ensure we have started
  if (!started_) {
    Start();

    if (!started_) {
      qWarning() << tr("An error occurred starting the Renderer node");
      return;
    }
  }
  */
}

void RendererProcessor::Release()
{
  Stop();
}

void RendererProcessor::InvalidateCache(const rational &start_range, const rational &end_range)
{
  qDebug() << "[RendererProcessor] Cache invalidated between"
           << start_range.toDouble()
           << "and"
           << end_range.toDouble();

  // FIXME: Snap start_range to timebase

  for (rational r=start_range;r<=end_range;r+=timebase_) {
    if (!cache_queue_.contains(r)) {
      cache_queue_.append(r);
    }
  }

  CacheNext();

  Node::InvalidateCache(start_range, end_range);
}

void RendererProcessor::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();
}

void RendererProcessor::SetParameters(const int &width,
                                      const int &height,
                                      const olive::PixelFormat &format,
                                      const olive::RenderMode &mode)
{
  // Since we're changing parameters, all the existing threads are invalid and must be removed. They will start again
  // next time this Node has to process anything.
  Stop();

  // Set new parameters
  width_ = width;
  height_ = height;
  format_ = format;
  mode_ = mode;

  // Regenerate the cache ID
  GenerateCacheIDInternal();
}

void RendererProcessor::Start()
{
  if (started_) {
    return;
  }

  threads_.resize(QThread::idealThreadCount());

  for (int i=0;i<threads_.size();i++) {
    threads_[i] = std::make_shared<RendererThread>(width_, height_, format_, mode_);
    threads_[i]->StartThread(QThread::HighPriority);

    // Ensure this connection is "Queued" so that it always runs in this object's threaded rather than any of the
    // other threads
    connect(threads_[i].get(), SIGNAL(FinishedPath()), this, SLOT(ThreadCallback()), Qt::QueuedConnection);
  }

  started_ = true;
}

void RendererProcessor::Stop()
{
  if (!started_) {
    return;
  }

  started_ = false;

  for (int i=0;i<threads_.size();i++) {
    threads_[i]->Cancel();
  }

  threads_.clear();
}

void RendererProcessor::GenerateCacheIDInternal()
{
  if (cache_name_.isEmpty() || width_ == 0 || height_ == 0) {
    return;
  }

  // Generate an ID that is more or less guaranteed to be unique to this Sequence
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(cache_name_.toUtf8());
  hash.addData(QString::number(cache_time_).toUtf8());
  hash.addData(QString::number(width_).toUtf8());
  hash.addData(QString::number(height_).toUtf8());
  hash.addData(QString::number(format_).toUtf8());

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

  rational time_to_cache = cache_queue_.takeFirst();

  qDebug() << "Caching" << time_to_cache.toDouble();

  Node* node_to_cache = texture_input_->edges().first()->output()->parent();

  // Run this probe in another thread
  RenderPath path = RendererProbe::ProbeNode(node_to_cache, threads_.size(), time_to_cache);

  cache_return_count_ = 0;

  for (int i=0;i<threads_.size();i++) {
    threads_.at(i)->Queue(path.GetThreadPath(i), time_to_cache);
  }

  caching_ = true;

  /*


  bool caching = false;

  // Look for a thread that's available
  for (int i=0;i<threads_.size();i++) {
    RendererThreadPtr thread = threads_.at(i);

    if (thread->Queue(node_to_cache, time_to_cache)) {
      // This thread is free and we've just taken control of it
      caching = true;
      break;
    }
  }

  if (caching) {
    cache_queue_.removeFirst();

    qDebug() << "[RendererProcessor] Ready to cache" << time_to_cache.numerator() << "/" << time_to_cache.denominator();
  }
  */
}

void RendererProcessor::ThreadCallback()
{
  cache_return_count_++;

  if (cache_return_count_ == threads_.size()) {
    // Threads are all done now, time to proceed
    caching_ = false;

    CacheNext();
  }
}

RendererThread* RendererProcessor::CurrentThread()
{
  return dynamic_cast<RendererThread*>(QThread::currentThread());
}

RenderInstance *RendererProcessor::CurrentInstance()
{
  RendererThread* thread = CurrentThread();

  if (thread != nullptr) {
    return thread->render_instance();
  }

  return nullptr;
}

NodeInput *RendererProcessor::texture_input()
{
  return texture_input_;
}

NodeOutput *RendererProcessor::texture_output()
{
  return texture_output_;
}
