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

#ifndef ENCODER_H
#define ENCODER_H

#include <memory>
#include <QString>

#include "codec/frame.h"
#include "common/timerange.h"
#include "render/audioparams.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class Encoder;
using EncoderPtr = std::shared_ptr<Encoder>;

class EncodingParams {
public:
  EncodingParams();

  void SetFilename(const QString& filename);

  void EnableVideo(const VideoRenderingParams& video_params, const QString& vcodec);
  void EnableAudio(const AudioRenderingParams& audio_params, const QString& acodec);

  void set_video_option(const QString& key, const QString& value);
  void set_video_bit_rate(const int64_t& rate);
  void set_video_max_bit_rate(const int64_t& rate);
  void set_video_buffer_size(const int64_t& sz);
  void set_video_threads(const int& threads);

  const QString& filename() const;

  bool video_enabled() const;
  const QString& video_codec() const;
  const VideoRenderingParams& video_params() const;
  const QHash<QString, QString>& video_opts() const;
  const int64_t& video_bit_rate() const;
  const int64_t& video_max_bit_rate() const;
  const int64_t& video_buffer_size() const;
  const int& video_threads() const;

  bool audio_enabled() const;
  const QString& audio_codec() const;
  const AudioRenderingParams& audio_params() const;

  const rational& GetExportLength() const;
  void SetExportLength(const rational& GetExportLength);

private:
  QString filename_;

  bool video_enabled_;
  QString video_codec_;
  VideoRenderingParams video_params_;
  QHash<QString, QString> video_opts_;
  int64_t video_bit_rate_;
  int64_t video_max_bit_rate_;
  int64_t video_buffer_size_;
  int video_threads_;

  bool audio_enabled_;
  QString audio_codec_;
  AudioRenderingParams audio_params_;

  rational export_length_;

};

class Encoder : public QObject
{
  Q_OBJECT
public:
  Encoder(const EncodingParams& params);

  /**
   * @brief Create a Encoder instance using a Encoder ID
   *
   * @return
   *
   * A Encoder instance or nullptr if a Decoder with this ID does not exist
   */
  static Encoder *CreateFromID(const QString& id, const EncodingParams &params);

  const EncodingParams& params() const;

public slots:
  void Open();
  void WriteFrame(OLIVE_NAMESPACE::FramePtr frame, OLIVE_NAMESPACE::rational time);
  virtual void WriteAudio(OLIVE_NAMESPACE::AudioRenderingParams pcm_info, const QString& pcm_filename, OLIVE_NAMESPACE::TimeRange range) = 0;
  void Close();

signals:
  void OpenSucceeded();
  void OpenFailed();

  void Closed();

  void AudioComplete();

protected:
  virtual bool OpenInternal() = 0;
  virtual void WriteInternal(FramePtr frame, rational time) = 0;
  virtual void CloseInternal() = 0;

  bool IsOpen() const;

private:
  EncodingParams params_;

  bool open_;
};

OLIVE_NAMESPACE_EXIT

#endif // ENCODER_H
