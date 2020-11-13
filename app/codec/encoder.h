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
#include <QXmlStreamWriter>

#include "codec/exportcodec.h"
#include "codec/exportformat.h"
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

  void EnableVideo(const VideoParams& video_params, const ExportCodec::Codec& vcodec);
  void EnableAudio(const AudioParams& audio_params, const ExportCodec::Codec &acodec);

  void set_video_option(const QString& key, const QString& value);
  void set_video_bit_rate(const int64_t& rate);
  void set_video_max_bit_rate(const int64_t& rate);
  void set_video_buffer_size(const int64_t& sz);
  void set_video_threads(const int& threads);
  void set_video_pix_fmt(const QString& s);

  const QString& filename() const;

  bool video_enabled() const;
  const ExportCodec::Codec& video_codec() const;
  const VideoParams& video_params() const;
  const QHash<QString, QString>& video_opts() const;
  const int64_t& video_bit_rate() const;
  const int64_t& video_max_bit_rate() const;
  const int64_t& video_buffer_size() const;
  const int& video_threads() const;
  const QString& video_pix_fmt() const;

  bool audio_enabled() const;
  const ExportCodec::Codec &audio_codec() const;
  const AudioParams& audio_params() const;

  const rational& GetExportLength() const;
  void SetExportLength(const rational& GetExportLength);

  virtual void Save(QXmlStreamWriter* writer) const;

private:
  QString filename_;

  bool video_enabled_;
  ExportCodec::Codec video_codec_;
  VideoParams video_params_;
  QHash<QString, QString> video_opts_;
  int64_t video_bit_rate_;
  int64_t video_max_bit_rate_;
  int64_t video_buffer_size_;
  int video_threads_;
  QString video_pix_fmt_;

  bool audio_enabled_;
  ExportCodec::Codec audio_codec_;
  AudioParams audio_params_;

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

  virtual bool Open() = 0;

  virtual bool WriteFrame(OLIVE_NAMESPACE::FramePtr frame, OLIVE_NAMESPACE::rational time) = 0;
  virtual void WriteAudio(OLIVE_NAMESPACE::AudioParams pcm_info,
                          QIODevice *file) = 0;
  void WriteAudio(OLIVE_NAMESPACE::AudioParams pcm_info,
                  const QString& pcm_filename);

  virtual void Close() = 0;

  virtual PixelFormat::Format GetDesiredPixelFormat() const
  {
    return PixelFormat::PIX_FMT_INVALID;
  }

private:
  EncodingParams params_;

};

OLIVE_NAMESPACE_EXIT

#endif // ENCODER_H
