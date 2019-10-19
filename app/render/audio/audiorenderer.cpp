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

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QtMath>

#include "common/filefunctions.h"
#include "render/pixelservice.h"

AudioRendererProcessor::AudioRendererProcessor() :
  started_(false),
  caching_(false),
  starting_(false)
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

QString AudioRendererProcessor::Name()
{
  return tr("Audio Renderer");
}

QString AudioRendererProcessor::Category()
{
  return tr("Processor");
}

QString AudioRendererProcessor::Description()
{
  return tr("A multi-threaded PCM audio renderer.");
}

QString AudioRendererProcessor::id()
{
  return "org.olivevideoeditor.Olive.rendererneptune";
}

void AudioRendererProcessor::SetCacheName(const QString &s)
{
  cache_name_ = s;
  cache_time_ = QDateTime::currentMSecsSinceEpoch();

  GenerateCacheIDInternal();
}

QVariant AudioRendererProcessor::Value(NodeOutput* output, const rational& time)
{
  Q_UNUSED(output)
  Q_UNUSED(time)

  /*if (output == texture_output_) {
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
        } else {
          qWarning() << "OIIO Error:" << OIIO::geterror().c_str();
        }
      }
    }
  }*/

  return 0;
}

void AudioRendererProcessor::Release()
{
  Stop();
}

void AudioRendererProcessor::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Q_UNUSED(from)

  if (timebase_.isNull()) {
    return;
  }

  //ClearCachedValuesInParameters(start_range, end_range);
  texture_input_->ClearCachedValue();
  length_input_->ClearCachedValue();

  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(length_input()->get_value(0).value<rational>(), end_range);

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
  }

  CacheNext();
}

void AudioRendererProcessor::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();
}

void AudioRendererProcessor::SetParameters(const int &sample_rate,
                                      const uint64_t &channel_layout,
                                      const olive::SampleFormat &format)
{
  // Since we're changing parameters, all the existing threads are invalid and must be removed. They will start again
  // next time this Node has to process anything.
  Stop();

  // Set new parameters
  sample_rate_ = sample_rate;
  channel_layout_ = channel_layout;
  format_ = format;

  // Regenerate the cache ID
  GenerateCacheIDInternal();
}

void AudioRendererProcessor::Start()
{
  if (started_) {
    return;
  }

  int background_thread_count = QThread::idealThreadCount();

  threads_.resize(background_thread_count);

  for (int i=0;i<threads_.size();i++) {
    threads_[i] = std::make_shared<AudioRendererThread>(sample_rate_, channel_layout_, format_);
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

  started_ = true;
}

void AudioRendererProcessor::Stop()
{
  if (!started_) {
    return;
  }

  started_ = false;

  foreach (AudioRendererThreadPtr process_thread, threads_) {
    process_thread->Cancel();
  }
  threads_.clear();
}

void AudioRendererProcessor::GenerateCacheIDInternal()
{
  if (cache_name_.isEmpty() || sample_rate_ == 0 || channel_layout_ == 0) {
    return;
  }

  // Generate an ID that is more or less guaranteed to be unique to this Sequence
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(cache_name_.toUtf8());
  hash.addData(QString::number(cache_time_).toUtf8());
  hash.addData(QString::number(sample_rate_).toUtf8());
  hash.addData(QString::number(channel_layout_).toUtf8());
  hash.addData(QString::number(format_).toUtf8());

  QByteArray bytes = hash.result();
  cache_id_ = bytes.toHex();
}

void AudioRendererProcessor::CacheNext()
{
  if (cache_queue_.isEmpty() || !texture_input_->IsConnected() || caching_) {
    return;
  }

  // Make sure cache has started
  Start();

  rational cache_frame = cache_queue_.takeFirst();

  qDebug() << "Caching" << cache_frame.toDouble();

  caching_ = true;
}

AudioRendererThreadBase *AudioRendererProcessor::CurrentThread()
{
  return dynamic_cast<AudioRendererThreadBase*>(QThread::currentThread());
}

AudioRendererParams *AudioRendererProcessor::CurrentInstance()
{
  AudioRendererThreadBase* thread = CurrentThread();

  if (thread != nullptr) {
    return thread->params();
  }

  return nullptr;
}

NodeInput *AudioRendererProcessor::texture_input()
{
  return texture_input_;
}

NodeInput *AudioRendererProcessor::length_input()
{
  return length_input_;
}

NodeOutput *AudioRendererProcessor::texture_output()
{
  return texture_output_;
}
