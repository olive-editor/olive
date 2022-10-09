/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QRegularExpression>
#include <QString>
#include <QXmlStreamWriter>

#include "codec/exportcodec.h"
#include "codec/exportformat.h"
#include "codec/frame.h"
#include "codec/samplebuffer.h"
#include "common/timerange.h"
#include "node/block/subtitle/subtitle.h"
#include "render/audioparams.h"
#include "render/colortransform.h"
#include "render/subtitleparams.h"
#include "render/videoparams.h"

namespace olive {

class Encoder;
using EncoderPtr = std::shared_ptr<Encoder>;

class EncodingParams
{
public:
  enum VideoScalingMethod {
    kFit,
    kStretch,
    kCrop
  };

  EncodingParams();

  static QDir GetPresetPath();
  static QStringList GetListOfPresets();

  bool IsValid() const
  {
    return video_enabled_ || audio_enabled_ || subtitles_enabled_;
  }

  void SetFilename(const QString& filename) { filename_ = filename; }

  void EnableVideo(const VideoParams& video_params, const ExportCodec::Codec& vcodec);
  void EnableAudio(const AudioParams& audio_params, const ExportCodec::Codec &acodec);
  void EnableSubtitles(const ExportCodec::Codec &scodec);
  void EnableSidecarSubtitles(const ExportFormat::Format &sfmt, const ExportCodec::Codec &scodec);

  void DisableVideo();
  void DisableAudio();
  void DisableSubtitles();

  const ExportFormat::Format &format() const { return format_; }
  void set_format(const ExportFormat::Format &format) { format_ = format; }

  void set_video_option(const QString& key, const QString& value) { video_opts_.insert(key, value); }
  void set_video_bit_rate(const int64_t& rate) { video_bit_rate_ = rate; }
  void set_video_min_bit_rate(const int64_t& rate) { video_min_bit_rate_ = rate; }
  void set_video_max_bit_rate(const int64_t& rate) { video_max_bit_rate_ = rate; }
  void set_video_buffer_size(const int64_t& sz) { video_buffer_size_ = sz; }
  void set_video_threads(const int& threads) { video_threads_ = threads; }
  void set_video_pix_fmt(const QString& s) { video_pix_fmt_ = s; }
  void set_video_is_image_sequence(bool s) { video_is_image_sequence_ = s; }
  void set_color_transform(const ColorTransform& color_transform) { color_transform_ = color_transform; }

  const QString& filename() const { return filename_; }

  bool video_enabled() const { return video_enabled_; }
  const ExportCodec::Codec& video_codec() const { return video_codec_; }
  const VideoParams& video_params() const { return video_params_; }
  const QHash<QString, QString>& video_opts() const { return video_opts_; }
  QString video_option(const QString &key) const { return video_opts_.value(key); }
  bool has_video_opt(const QString &key) const { return video_opts_.contains(key); }
  const int64_t& video_bit_rate() const { return video_bit_rate_; }
  const int64_t& video_min_bit_rate() const { return video_min_bit_rate_; }
  const int64_t& video_max_bit_rate() const { return video_max_bit_rate_; }
  const int64_t& video_buffer_size() const { return video_buffer_size_; }
  const int& video_threads() const { return video_threads_; }
  const QString& video_pix_fmt() const { return video_pix_fmt_; }
  bool video_is_image_sequence() const { return video_is_image_sequence_; }
  const ColorTransform& color_transform() const { return color_transform_; }

  bool audio_enabled() const { return audio_enabled_; }
  const ExportCodec::Codec &audio_codec() const { return audio_codec_; }
  const AudioParams& audio_params() const { return audio_params_; }
  const int64_t& audio_bit_rate() const { return audio_bit_rate_; }

  void set_audio_bit_rate(const int64_t& b) { audio_bit_rate_ = b; }

  bool subtitles_enabled() const { return subtitles_enabled_; }
  bool subtitles_are_sidecar() const { return subtitles_are_sidecar_; }
  ExportFormat::Format subtitle_sidecar_fmt() const { return subtitle_sidecar_fmt_; }
  ExportCodec::Codec subtitles_codec() const { return subtitles_codec_; }

  const rational& GetExportLength() const { return export_length_; }
  void SetExportLength(const rational& export_length) { export_length_ = export_length; }

  bool Load(QIODevice *device);
  bool Load(QXmlStreamReader *reader);

  void Save(QIODevice *device) const;
  void Save(QXmlStreamWriter* writer) const;

  bool has_custom_range() const { return has_custom_range_; }
  const TimeRange& custom_range() const { return custom_range_; }
  void set_custom_range(const TimeRange& custom_range)
  {
    has_custom_range_ = true;
    custom_range_ = custom_range;
  }

  const VideoScalingMethod& video_scaling_method() const { return video_scaling_method_; }
  void set_video_scaling_method(const VideoScalingMethod& video_scaling_method) { video_scaling_method_ = video_scaling_method; }

  static QMatrix4x4 GenerateMatrix(VideoScalingMethod method,
                                   int source_width, int source_height,
                                   int dest_width, int dest_height);

private:
  static const int kEncoderParamsVersion = 1;

  bool LoadV1(QXmlStreamReader *reader);

  QString filename_;
  ExportFormat::Format format_;

  bool video_enabled_;
  ExportCodec::Codec video_codec_;
  VideoParams video_params_;
  QHash<QString, QString> video_opts_;
  int64_t video_bit_rate_;
  int64_t video_min_bit_rate_;
  int64_t video_max_bit_rate_;
  int64_t video_buffer_size_;
  int video_threads_;
  QString video_pix_fmt_;
  bool video_is_image_sequence_;
  ColorTransform color_transform_;

  bool audio_enabled_;
  ExportCodec::Codec audio_codec_;
  AudioParams audio_params_;
  int64_t audio_bit_rate_;

  bool subtitles_enabled_;
  bool subtitles_are_sidecar_;
  ExportFormat::Format subtitle_sidecar_fmt_;
  ExportCodec::Codec subtitles_codec_;

  rational export_length_;
  VideoScalingMethod video_scaling_method_;

  bool has_custom_range_;
  TimeRange custom_range_;

};

class Encoder : public QObject
{
  Q_OBJECT
public:
  Encoder(const EncodingParams& params);

  enum Type {
    kEncoderTypeNone = -1,
    kEncoderTypeFFmpeg,
    kEncoderTypeOIIO
  };

  /**
   * @brief Create a Encoder instance using a Encoder ID
   *
   * @return
   *
   * A Encoder instance or nullptr if a Decoder with this ID does not exist
   */
  static Encoder *CreateFromID(Type id, const EncodingParams &params);

  static Type GetTypeFromFormat(ExportFormat::Format f);

  static Encoder *CreateFromFormat(ExportFormat::Format f, const EncodingParams &params);

  static Encoder *CreateFromParams(const EncodingParams &params);

  virtual QStringList GetPixelFormatsForCodec(ExportCodec::Codec c) const;
  virtual std::vector<AudioParams::Format> GetSampleFormatsForCodec(ExportCodec::Codec c) const;

  const EncodingParams& params() const;

  virtual VideoParams::Format GetDesiredPixelFormat() const
  {
    return VideoParams::kFormatInvalid;
  }

  const QString& GetError() const
  {
    return error_;
  }

  QString GetFilenameForFrame(const rational& frame);

  static int GetImageSequencePlaceholderDigitCount(const QString& filename);

  static bool FilenameContainsDigitPlaceholder(const QString &filename);
  static QString FilenameRemoveDigitPlaceholder(QString filename);

  static const QRegularExpression kImageSequenceContainsDigits;
  static const QRegularExpression kImageSequenceRemoveDigits;

public slots:
  virtual bool Open() = 0;

  virtual bool WriteFrame(olive::FramePtr frame, olive::rational time) = 0;
  virtual bool WriteAudio(const olive::SampleBuffer &audio) = 0;
  virtual bool WriteSubtitle(const SubtitleBlock *sub_block) = 0;

  virtual void Close() = 0;

protected:
  void SetError(const QString& err)
  {
    error_ = err;
  }

private:
  EncodingParams params_;

  QString error_;

};

}

#endif // ENCODER_H
