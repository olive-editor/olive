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

#include "encoder.h"

#include <QFile>

#include "common/timecodefunctions.h"
#include "ffmpeg/ffmpegencoder.h"
#include "oiio/oiioencoder.h"

namespace olive {

const QRegularExpression Encoder::kImageSequenceContainsDigits = QRegularExpression(QStringLiteral("\\[[#]+\\]"));
const QRegularExpression Encoder::kImageSequenceRemoveDigits = QRegularExpression(QStringLiteral("[\\-\\.\\ \\_]?\\[[#]+\\]"));

Encoder::Encoder(const EncodingParams &params) :
  params_(params)
{
}

const EncodingParams &Encoder::params() const
{
  return params_;
}

QString Encoder::GetFilenameForFrame(const rational &frame)
{
  if (params().video_is_image_sequence()) {
    // Transform!
    int64_t frame_index = Timecode::time_to_timestamp(frame, params().video_params().frame_rate_as_time_base());
    int digits = GetImageSequencePlaceholderDigitCount(params().filename());
    QString frame_index_str = QStringLiteral("%1").arg(frame_index, digits, 10, QChar('0'));

    QString f = params_.filename();
    f.replace(kImageSequenceContainsDigits, frame_index_str);
    return f;
  } else {
    // Keep filename
    return params_.filename();
  }
}

int Encoder::GetImageSequencePlaceholderDigitCount(const QString &filename)
{
  int start = filename.indexOf(kImageSequenceContainsDigits);
  int digit_count = 0;
  for (int i=start+1; i<filename.size(); i++) {
    if (filename.at(i) == '#') {
      digit_count++;
    } else {
      break;
    }
  }
  return digit_count;
}

bool Encoder::FilenameContainsDigitPlaceholder(const QString& filename)
{
  return filename.contains(kImageSequenceContainsDigits);
}

QString Encoder::FilenameRemoveDigitPlaceholder(QString filename)
{
  return filename.remove(kImageSequenceRemoveDigits);
}

EncodingParams::EncodingParams() :
  video_enabled_(false),
  video_bit_rate_(0),
  video_min_bit_rate_(0),
  video_max_bit_rate_(0),
  video_buffer_size_(0),
  video_threads_(0),
  video_is_image_sequence_(false),
  video_color_range_(kYUVDefault),
  audio_enabled_(false),
  audio_bit_rate_(0),
  subtitles_enabled_(false)
{
}

void EncodingParams::EnableVideo(const VideoParams &video_params, const ExportCodec::Codec &vcodec)
{
  video_enabled_ = true;
  video_params_ = video_params;
  video_codec_ = vcodec;
}

void EncodingParams::EnableAudio(const AudioParams &audio_params, const ExportCodec::Codec &acodec)
{
  audio_enabled_ = true;
  audio_params_ = audio_params;
  audio_codec_ = acodec;
}

void EncodingParams::EnableSubtitles(const ExportCodec::Codec &scodec)
{
  subtitles_enabled_ = true;
  subtitles_codec_ = scodec;
}

void EncodingParams::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("filename"), filename_);

  writer->writeStartElement(QStringLiteral("video"));

  writer->writeAttribute(QStringLiteral("enabled"), QString::number(video_enabled_));

  if (video_enabled_) {
    writer->writeTextElement(QStringLiteral("codec"), QString::number(video_codec_));
    writer->writeTextElement(QStringLiteral("width"), QString::number(video_params_.width()));
    writer->writeTextElement(QStringLiteral("height"), QString::number(video_params_.height()));
    writer->writeTextElement(QStringLiteral("format"), QString::number(video_params_.format()));
    writer->writeTextElement(QStringLiteral("timebase"), video_params_.time_base().toString());
    writer->writeTextElement(QStringLiteral("divider"), QString::number(video_params_.divider()));
    writer->writeTextElement(QStringLiteral("bitrate"), QString::number(video_bit_rate_));
    writer->writeTextElement(QStringLiteral("minbitrate"), QString::number(video_max_bit_rate_));
    writer->writeTextElement(QStringLiteral("maxbitrate"), QString::number(video_max_bit_rate_));
    writer->writeTextElement(QStringLiteral("bufsize"), QString::number(video_buffer_size_));
    writer->writeTextElement(QStringLiteral("threads"), QString::number(video_threads_));

    if (!video_opts_.isEmpty()) {
      writer->writeStartElement(QStringLiteral("opts"));

      QHash<QString, QString>::const_iterator i;
      for (i=video_opts_.constBegin(); i!=video_opts_.constEnd(); i++) {
        writer->writeStartElement(QStringLiteral("entry"));

        writer->writeTextElement(QStringLiteral("key"), i.key());
        writer->writeTextElement(QStringLiteral("value"), i.value());

        writer->writeEndElement(); // entry
      }

      writer->writeEndElement(); // opts
    }
  }

  writer->writeEndElement(); // video

  writer->writeStartElement(QStringLiteral("audio"));

  writer->writeAttribute(QStringLiteral("enabled"), QString::number(audio_enabled_));

  if (audio_enabled_) {
    writer->writeTextElement(QStringLiteral("codec"), QString::number(audio_codec_));
    writer->writeTextElement(QStringLiteral("samplerate"), QString::number(audio_params_.sample_rate()));
    writer->writeTextElement(QStringLiteral("channellayout"), QString::number(audio_params_.channel_layout()));
    writer->writeTextElement(QStringLiteral("format"), QString::number(audio_params_.format()));
  }

  writer->writeEndElement(); // audio
}

Encoder* Encoder::CreateFromID(Type id, const EncodingParams& params)
{
  switch (id) {
  case kEncoderTypeNone:
    break;
  case kEncoderTypeFFmpeg:
    return new FFmpegEncoder(params);
  case kEncoderTypeOIIO:
    return new OIIOEncoder(params);
  }

  return nullptr;
}

Encoder::Type Encoder::GetTypeFromFormat(ExportFormat::Format f)
{
  switch (f) {
  case ExportFormat::kFormatDNxHD:
  case ExportFormat::kFormatMatroska:
  case ExportFormat::kFormatQuickTime:
  case ExportFormat::kFormatMPEG4Video:
  case ExportFormat::kFormatMPEG4Audio:
  case ExportFormat::kFormatWAV:
  case ExportFormat::kFormatAIFF:
  case ExportFormat::kFormatMP3:
  case ExportFormat::kFormatFLAC:
  case ExportFormat::kFormatOgg:
  case ExportFormat::kFormatWebM:
  case ExportFormat::kFormatSRT:
    return kEncoderTypeFFmpeg;
  case ExportFormat::kFormatOpenEXR:
  case ExportFormat::kFormatPNG:
  case ExportFormat::kFormatTIFF:
    return kEncoderTypeOIIO;
  case ExportFormat::kFormatCount:
    break;
  }

  return kEncoderTypeNone;
}

Encoder *Encoder::CreateFromFormat(ExportFormat::Format f, const EncodingParams &params)
{
  return CreateFromID(GetTypeFromFormat(f), params);
}

QStringList Encoder::GetPixelFormatsForCodec(ExportCodec::Codec c) const
{
  return QStringList();
}

std::vector<AudioParams::Format> Encoder::GetSampleFormatsForCodec(ExportCodec::Codec c) const
{
  return std::vector<AudioParams::Format>();
}

}
