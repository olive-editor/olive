/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "previewgenerator.h"

#include "ui/mediaiconservice.h"
#include "project/media.h"
#include "project/footage.h"
#include "panels/viewer.h"
#include "panels/project.h"
#include "io/config.h"
#include "io/path.h"
#include "debug.h"

#include <QPainter>
#include <QPixmap>
#include <QtMath>
#include <QTreeWidgetItem>
#include <QSemaphore>
#include <QFile>
#include <QDir>

QSemaphore sem(5); // only 5 preview generators can run at one time

PreviewGenerator::PreviewGenerator(Media* i) :
  QThread(nullptr)
{
  fmt_ctx_ = (nullptr);
  media_ = (i);
  retrieve_duration_ = (false);
  contains_still_image_ = (false);
  cancelled_ = (false);
  footage_ = media_->to_footage();

  footage_->preview_gen = this;

  data_dir_ = QDir(get_data_dir().filePath("previews"));
  if (!data_dir_.exists()) {
    data_dir_.mkpath(".");
  }

  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));

  // set up throbber animation
  olive::media_icon_service->SetMediaIcon(media_, ICON_TYPE_LOADING);

  start(QThread::LowPriority);
}

void PreviewGenerator::parse_media() {
  // detect video/audio streams in file
  for (int i=0;i<int(fmt_ctx_->nb_streams);i++) {
    // Find the decoder for the video stream
    if (avcodec_find_decoder(fmt_ctx_->streams[i]->codecpar->codec_id) == nullptr) {
      qCritical() << "Unsupported codec in stream" << i << "of file" << footage_->name;
    } else {
      FootageStream ms;
      ms.preview_done = false;
      ms.file_index = i;
      ms.enabled = true;
      ms.infinite_length = false;

      bool append = false;

      if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
          && fmt_ctx_->streams[i]->codecpar->width > 0
          && fmt_ctx_->streams[i]->codecpar->height > 0) {

        // heuristic to determine if video is a still image (if it is, we treat it differently in the playback/render process)
        if (fmt_ctx_->streams[i]->avg_frame_rate.den == 0
            && fmt_ctx_->streams[i]->codecpar->codec_id != AV_CODEC_ID_DNXHD) { // silly hack but this is the only scenario i've seen this
          if (footage_->url.contains('%')) {
            // must be an image sequence
            ms.video_frame_rate = 25;
          } else {
            ms.infinite_length = true;
            contains_still_image_ = true;
            ms.video_frame_rate = 0;
          }

        } else {
          // using ffmpeg's built-in heuristic
          ms.video_frame_rate = av_q2d(av_guess_frame_rate(fmt_ctx_, fmt_ctx_->streams[i], nullptr));
        }

        ms.video_width = fmt_ctx_->streams[i]->codecpar->width;
        ms.video_height = fmt_ctx_->streams[i]->codecpar->height;

        // default value, we get the true value later in generate_waveform()
        ms.video_auto_interlacing = VIDEO_PROGRESSIVE;
        ms.video_interlacing = VIDEO_PROGRESSIVE;

        append = true;
      } else if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        ms.audio_channels = fmt_ctx_->streams[i]->codecpar->channels;
        ms.audio_layout = int(fmt_ctx_->streams[i]->codecpar->channel_layout);
        ms.audio_frequency = fmt_ctx_->streams[i]->codecpar->sample_rate;

        append = true;
      }

      if (append) {
        QVector<FootageStream>& stream_list = (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) ?
              footage_->audio_tracks : footage_->video_tracks;

        for (int j=0;j<stream_list.size();j++) {
          if (stream_list.at(j).file_index == i) {
            stream_list[j] = ms;
            append = false;
          }
        }

        if (append) stream_list.append(ms);
      }
    }
  }
  footage_->length = fmt_ctx_->duration;

  if (fmt_ctx_->duration == INT64_MIN) {
    retrieve_duration_ = true;
  } else {
    finalize_media();
  }
}

bool PreviewGenerator::retrieve_preview(const QString& hash) {
  // returns true if generate_waveform must be run, false if we got all previews from cached files
  if (retrieve_duration_) {
    //dout << "[NOTE] " << media->name << "needs to retrieve duration";
    return true;
  }

  bool found = true;
  for (int i=0;i<footage_->video_tracks.size();i++) {
    FootageStream& ms = footage_->video_tracks[i];
    QString thumb_path = get_thumbnail_path(hash, ms);
    QFile f(thumb_path);
    if (f.exists() && ms.video_preview.load(thumb_path)) {
      //dout << "loaded thumb" << ms->file_index << "from" << thumb_path;
      ms.make_square_thumb();
      ms.preview_done = true;
    } else {
      found = false;
      break;
    }
  }
  for (int i=0;i<footage_->audio_tracks.size();i++) {
    FootageStream& ms = footage_->audio_tracks[i];
    QString waveform_path = get_waveform_path(hash, ms);
    QFile f(waveform_path);
    if (f.exists()) {
      //dout << "loaded wave" << ms->file_index << "from" << waveform_path;
      f.open(QFile::ReadOnly);
      QByteArray data = f.readAll();
      ms.audio_preview.resize(data.size());
      for (int j=0;j<data.size();j++) {
        // faster way?
        ms.audio_preview[j] = data.at(j);
      }
      ms.preview_done = true;
      f.close();
    } else {
      found = false;
      break;
    }
  }
  if (!found) {
    for (int i=0;i<footage_->video_tracks.size();i++) {
      FootageStream& ms = footage_->video_tracks[i];
      ms.preview_done = false;
    }
    for (int i=0;i<footage_->audio_tracks.size();i++) {
      FootageStream& ms = footage_->audio_tracks[i];
      ms.audio_preview.clear();
      ms.preview_done = false;
    }
  }
  return !found;
}

void PreviewGenerator::finalize_media() {
  if (!cancelled_) {
    bool footage_is_ready = true;

    if (footage_->video_tracks.isEmpty() && footage_->audio_tracks.isEmpty()) {
      // ERROR
      footage_is_ready = false;
      invalidate_media(tr("Failed to find any valid video/audio streams"));
    } else if (!footage_->video_tracks.isEmpty() && !contains_still_image_) {
      // VIDEO
      olive::media_icon_service->SetMediaIcon(media_, ICON_TYPE_VIDEO);
    } else if (!footage_->audio_tracks.isEmpty()) {
      // AUDIO
      olive::media_icon_service->SetMediaIcon(media_, ICON_TYPE_AUDIO);
    } else {
      // IMAGE
      olive::media_icon_service->SetMediaIcon(media_, ICON_TYPE_IMAGE);
    }

    if (footage_is_ready) {
      footage_->ready_lock.unlock();
      footage_->ready = true;
      media_->update_tooltip();
    }

    if (olive::ActiveSequence != nullptr) {
      olive::ActiveSequence->RefreshClips(media_);
    }
  }
}

void PreviewGenerator::invalidate_media(const QString &error_msg)
{
  media_->update_tooltip(error_msg);
  olive::media_icon_service->SetMediaIcon(media_, ICON_TYPE_ERROR);
  footage_->invalid = true;
  footage_->ready_lock.unlock();
}

void PreviewGenerator::generate_waveform() {
  SwsContext* sws_ctx;
  SwrContext* swr_ctx;
  AVFrame* temp_frame = av_frame_alloc();

  // stores codec contexts for format's streams
  AVCodecContext** codec_ctx = new AVCodecContext* [fmt_ctx_->nb_streams];

  // stores media lengths while scanning in case the format has no duration metadata
  int64_t* media_lengths = new int64_t[fmt_ctx_->nb_streams]{0};

  // stores samples while scanning before they get sent to preview file
  qint16*** waveform_cache_data = new qint16** [fmt_ctx_->nb_streams];
  int waveform_cache_count = 0;

  // defaults to false, sets to true if we find a valid stream to make a preview of
  bool create_previews = false;

  for (unsigned int i=0;i<fmt_ctx_->nb_streams;i++) {

    // default to nullptr values for easier memory management later
    codec_ctx[i] = nullptr;
    waveform_cache_data[i] = nullptr;

    // we only generate previews for video and audio
    // and only if the thumbnail and waveform sizes are > 0
    if ((fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && olive::CurrentConfig.thumbnail_resolution > 0)
        || (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && olive::CurrentConfig.waveform_resolution > 0)) {
      AVCodec* codec = avcodec_find_decoder(fmt_ctx_->streams[i]->codecpar->codec_id);
      if (codec != nullptr) {

        // alloc the context and load the params into it
        codec_ctx[i] = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_ctx[i], fmt_ctx_->streams[i]->codecpar);

        // open the decoder
        avcodec_open2(codec_ctx[i], codec, nullptr);

        // audio specific functions
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {

          // allocate sample cache for this stream
          waveform_cache_data[i] = new qint16* [fmt_ctx_->streams[i]->codecpar->channels];

          // each channel gets a min and a max value so we allocate two ints for each one
          for (int j=0;j<fmt_ctx_->streams[i]->codecpar->channels;j++) {
            waveform_cache_data[i][j] = new qint16[2];
          }

          // if codec context has no defined channel layout, guess it from the channel count
          if (codec_ctx[i]->channel_layout == 0) {
            codec_ctx[i]->channel_layout = av_get_default_channel_layout(fmt_ctx_->streams[i]->codecpar->channels);
          }

        }

        // enable next step of process
        create_previews = true;
      }
    }
  }

  if (create_previews) {
    // TODO may be unnecessary - doesn't av_read_frame allocate a packet itself?
    AVPacket* packet = av_packet_alloc();

    bool done = true;

    bool end_of_file = false;

    // get the ball rolling
    do {
      av_read_frame(fmt_ctx_, packet);
    } while (codec_ctx[packet->stream_index] == nullptr);
    avcodec_send_packet(codec_ctx[packet->stream_index], packet);

    while (!end_of_file) {
      while (codec_ctx[packet->stream_index] == nullptr || avcodec_receive_frame(codec_ctx[packet->stream_index], temp_frame) == AVERROR(EAGAIN)) {
        av_packet_unref(packet);
        int read_ret = av_read_frame(fmt_ctx_, packet);

        if (read_ret < 0) {
          end_of_file = true;
          if (read_ret != AVERROR_EOF) qCritical() << "Failed to read packet for preview generation" << read_ret;
          break;
        }
        if (codec_ctx[packet->stream_index] != nullptr) {
          int send_ret = avcodec_send_packet(codec_ctx[packet->stream_index], packet);
          if (send_ret < 0 && send_ret != AVERROR(EAGAIN)) {
            qCritical() << "Failed to send packet for preview generation - aborting" << send_ret;
            end_of_file = true;
            break;
          }
        }
      }
      if (!end_of_file) {
        FootageStream* s = footage_->get_stream_from_file_index(fmt_ctx_->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO, packet->stream_index);
        if (s != nullptr) {
          if (fmt_ctx_->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!s->preview_done) {
              int dstH = olive::CurrentConfig.thumbnail_resolution;
              int dstW = qRound(dstH * (float(temp_frame->width)/float(temp_frame->height)));

              sws_ctx = sws_getContext(
                    temp_frame->width,
                    temp_frame->height,
                    static_cast<AVPixelFormat>(temp_frame->format),
                    dstW,
                    dstH,
                    static_cast<AVPixelFormat>(AV_PIX_FMT_RGBA),
                    SWS_FAST_BILINEAR,
                    nullptr,
                    nullptr,
                    nullptr
                    );

              int linesize[AV_NUM_DATA_POINTERS];
              linesize[0] = dstW*4;

              s->video_preview = QImage(dstW, dstH, QImage::Format_RGBA8888);
              uint8_t* data = s->video_preview.bits();

              sws_scale(sws_ctx,
                        temp_frame->data,
                        temp_frame->linesize,
                        0,
                        temp_frame->height,
                        &data,
                        linesize);

              s->make_square_thumb();

              // is video interlaced?
              s->video_auto_interlacing = (temp_frame->interlaced_frame) ? ((temp_frame->top_field_first) ? VIDEO_TOP_FIELD_FIRST : VIDEO_BOTTOM_FIELD_FIRST) : VIDEO_PROGRESSIVE;
              s->video_interlacing = s->video_auto_interlacing;

              s->preview_done = true;

              sws_freeContext(sws_ctx);

              if (!retrieve_duration_) {
                avcodec_close(codec_ctx[packet->stream_index]);
                codec_ctx[packet->stream_index] = nullptr;
              }
            }
            media_lengths[packet->stream_index]++;
          } else if (fmt_ctx_->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AVFrame* swr_frame = av_frame_alloc();
            swr_frame->channel_layout = temp_frame->channel_layout;
            swr_frame->sample_rate = temp_frame->sample_rate;
            swr_frame->format = AV_SAMPLE_FMT_S16P;

            swr_ctx = swr_alloc_set_opts(
                  nullptr,
                  temp_frame->channel_layout,
                  static_cast<AVSampleFormat>(swr_frame->format),
                  temp_frame->sample_rate,
                  temp_frame->channel_layout,
                  static_cast<AVSampleFormat>(temp_frame->format),
                  temp_frame->sample_rate,
                  0,
                  nullptr
                  );

            swr_init(swr_ctx);

            swr_convert_frame(swr_ctx, swr_frame, temp_frame);

            // `config.waveform_resolution` determines how many samples per second are stored in waveform.
            // `sample_rate` is samples per second, so `interval` is how many samples are averaged in
            // each "point" of the waveform
            int interval = qFloor((temp_frame->sample_rate/olive::CurrentConfig.waveform_resolution)/4)*4;

            // get the amount of bytes in an audio sample
            int sample_size = av_get_bytes_per_sample(static_cast<AVSampleFormat>(swr_frame->format));

            // total amount of data in this frame
            int nb_bytes = swr_frame->nb_samples * sample_size;

            // loop through entire frame
            for (int i=0;i<nb_bytes;i+=sample_size) {

              // check if we've hit sample threshold
              if (waveform_cache_count == interval) {

                // if so, we dump our cached values into the preview and reset them
                // for the next interval
                for (int j=0;j<swr_frame->channels;j++) {
                  qint16& min = waveform_cache_data[packet->stream_index][j][0];
                  qint16& max = waveform_cache_data[packet->stream_index][j][1];

                  s->audio_preview.append(min >> 8);
                  s->audio_preview.append(max >> 8);
                }

                waveform_cache_count = 0;
              }

              // standard processing for each channel of information
              for (int j=0;j<swr_frame->channels;j++) {
                qint16& min = waveform_cache_data[packet->stream_index][j][0];
                qint16& max = waveform_cache_data[packet->stream_index][j][1];

                // if we're starting over, reset cache to zero
                if (waveform_cache_count == 0) {
                  min = 0;
                  max = 0;
                }

                // store most minimum and most maximum samples of this interval
                qint16 sample = qint16((swr_frame->data[j][i+1] << 8) | swr_frame->data[j][i]);
                min = qMin(min, sample);
                max = qMax(max, sample);
              }

              waveform_cache_count++;

              if (cancelled_) {
                break;
              }
            }

            swr_free(&swr_ctx);
            av_frame_free(&swr_frame);

            if (cancelled_) {
              end_of_file = true;
              break;
            }
          }
        }

        // check if we've got all our previews
        if (retrieve_duration_) {
          done = false;
        } else if (footage_->audio_tracks.size() == 0) {
          done = true;
          for (int i=0;i<footage_->video_tracks.size();i++) {
            if (!footage_->video_tracks.at(i).preview_done) {
              done = false;
              break;
            }
          }
          if (done) {
            end_of_file = true;
            break;
          }
        }
        av_packet_unref(packet);
      }
    }

    av_frame_free(&temp_frame);
    av_packet_free(&packet);

    for (unsigned int i=0;i<fmt_ctx_->nb_streams;i++) {
      if (waveform_cache_data[i] != nullptr) {
        for (int j=0;j<codec_ctx[i]->channels;j++) {
          delete [] waveform_cache_data[i][j];
        }
        delete [] waveform_cache_data[i];
      }

      if (codec_ctx[i] != nullptr) {
        avcodec_close(codec_ctx[i]);
        avcodec_free_context(&codec_ctx[i]);
      }
    }

    // by this point, we'll have made all audio waveform previews
    for (int i=0;i<footage_->audio_tracks.size();i++) {
      footage_->audio_tracks[i].preview_done = true;
    }
  }

  if (retrieve_duration_) {
    footage_->length = 0;
    unsigned int maximum_stream = 0;
    for (unsigned int i=0;i<fmt_ctx_->nb_streams;i++) {
      if (media_lengths[i] > media_lengths[maximum_stream]) {
        maximum_stream = i;
      }
    }

    // FIXME: length is currently retrieved as a frame count rather than a timestamp
    footage_->length = qRound(double(media_lengths[maximum_stream]) / av_q2d(fmt_ctx_->streams[maximum_stream]->avg_frame_rate) * AV_TIME_BASE);

    finalize_media();
  }

  delete [] waveform_cache_data;
  delete [] media_lengths;
  delete [] codec_ctx;
}

QString PreviewGenerator::get_thumbnail_path(const QString& hash, const FootageStream& ms) {
  return data_dir_.filePath(QString("%1t%2").arg(hash, QString::number(ms.file_index)));
}

QString PreviewGenerator::get_waveform_path(const QString& hash, const FootageStream& ms) {
  return data_dir_.filePath(QString("%1w%2").arg(hash, QString::number(ms.file_index)));
}

void PreviewGenerator::run() {
  Q_ASSERT(footage_ != nullptr);
  Q_ASSERT(media_ != nullptr);

  const QString url = footage_->url;
  QByteArray ba = url.toUtf8();
  char* filename = new char[ba.size()+1];
  strcpy(filename, ba.data());

  QString errorStr;
  bool error = false;

  AVDictionary* format_opts = nullptr;

  // for image sequences that don't start at 0, set the index where it does start
  if (footage_->start_number > 0) {
    av_dict_set(&format_opts, "start_number", QString::number(footage_->start_number).toUtf8(), 0);
  }

  int errCode = avformat_open_input(&fmt_ctx_, filename, nullptr, &format_opts);
  if(errCode != 0) {
    char err[1024];
    av_strerror(errCode, err, 1024);
    errorStr = tr("Could not open file - %1").arg(err);
    error = true;
  } else {
    errCode = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (errCode < 0) {
      char err[1024];
      av_strerror(errCode, err, 1024);
      errorStr = tr("Could not find stream information - %1").arg(err);
      error = true;
    } else {
      av_dump_format(fmt_ctx_, 0, filename, 0);
      parse_media();

      // see if we already have data for this
      QString hash = get_file_hash(footage_->url);

      if (retrieve_preview(hash)) {
        sem.acquire();

        if (!cancelled_) {
          generate_waveform();

          if (!cancelled_) {
            // save preview to file
            for (int i=0;i<footage_->video_tracks.size();i++) {
              FootageStream& ms = footage_->video_tracks[i];
              ms.video_preview.save(get_thumbnail_path(hash, ms), "PNG");
              //dout << "saved" << ms->file_index << "thumbnail to" << get_thumbnail_path(hash, ms);
            }
            for (int i=0;i<footage_->audio_tracks.size();i++) {
              FootageStream& ms = footage_->audio_tracks[i];
              QFile f(get_waveform_path(hash, ms));
              f.open(QFile::WriteOnly);
              f.write(ms.audio_preview.constData(), ms.audio_preview.size());
              f.close();
              //dout << "saved" << ms->file_index << "waveform to" << get_waveform_path(hash, ms);
            }
          }
        }

        sem.release();
      }
    }
    avformat_close_input(&fmt_ctx_);
  }

  if (!cancelled_) {
    if (error) {
      invalidate_media(errorStr);
    }
  }

  delete [] filename;
  footage_->preview_gen = nullptr;
}

void PreviewGenerator::cancel() {
  cancelled_ = true;
  wait();
}

void PreviewGenerator::AnalyzeMedia(Media *m)
{
  // PreviewGenerator's constructor starts the thread, sets a reference of itself as the media's generator,
  // and connects its thread completion to its own deletion, therefore handling its own memory. Nothing else needs to
  // be done.
  new PreviewGenerator(m);
}
