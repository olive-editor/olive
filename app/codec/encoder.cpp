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

#include "common/filefunctions.h"
#include "common/xmlutils.h"
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
  audio_enabled_(false),
  audio_bit_rate_(0),
  subtitles_enabled_(false),
  subtitles_are_sidecar_(false),
  video_scaling_method_(kStretch),
  has_custom_range_(false)
{
}

QDir EncodingParams::GetPresetPath()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("exportpresets"));
}

QStringList EncodingParams::GetListOfPresets()
{
  QDir d = EncodingParams::GetPresetPath();
  return d.entryList(QDir::Files);
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

void EncodingParams::EnableSidecarSubtitles(const ExportFormat::Format &sfmt, const ExportCodec::Codec &scodec)
{
  subtitles_enabled_ = true;
  subtitles_are_sidecar_ = true;
  subtitle_sidecar_fmt_ = sfmt;
  subtitles_codec_ = scodec;
}

void EncodingParams::DisableVideo()
{
  video_enabled_ = false;
}

void EncodingParams::DisableAudio()
{
  audio_enabled_ = false;
}

void EncodingParams::DisableSubtitles()
{
  subtitles_enabled_ = false;
}

bool EncodingParams::Load(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("export")) {
      int version = 0;

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("version")) {
          version = attr.value().toInt();
        }
      }

      switch (version) {
      case 1:
        return LoadV1(reader);
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  return false;
}

bool EncodingParams::Load(QIODevice *device)
{
  QXmlStreamReader reader(device);
  return Load(&reader);
}

void EncodingParams::Save(QIODevice *device) const
{
  QXmlStreamWriter writer(device);
  Save(&writer);
}

void EncodingParams::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartDocument();

  writer->writeStartElement(QStringLiteral("export"));

  writer->writeAttribute(QStringLiteral("version"), QString::number(kEncoderParamsVersion));

  writer->writeTextElement(QStringLiteral("filename"), filename_);
  writer->writeTextElement(QStringLiteral("format"), QString::number(format_));

  writer->writeTextElement(QStringLiteral("range"), QString::number(has_custom_range_));
  writer->writeTextElement(QStringLiteral("customrangein"), QString::fromStdString(custom_range_.in().toString()));
  writer->writeTextElement(QStringLiteral("customrangeout"), QString::fromStdString(custom_range_.out().toString()));

  writer->writeStartElement(QStringLiteral("video"));

  writer->writeAttribute(QStringLiteral("enabled"), QString::number(video_enabled_));

  if (video_enabled_) {
    writer->writeTextElement(QStringLiteral("codec"), QString::number(video_codec_));
    writer->writeTextElement(QStringLiteral("width"), QString::number(video_params_.width()));
    writer->writeTextElement(QStringLiteral("height"), QString::number(video_params_.height()));
    writer->writeTextElement(QStringLiteral("format"), QString::number(video_params_.format()));
    writer->writeTextElement(QStringLiteral("pixelaspect"), QString::fromStdString(video_params_.pixel_aspect_ratio().toString()));
    writer->writeTextElement(QStringLiteral("timebase"), QString::fromStdString(video_params_.time_base().toString()));
    writer->writeTextElement(QStringLiteral("divider"), QString::number(video_params_.divider()));
    writer->writeTextElement(QStringLiteral("bitrate"), QString::number(video_bit_rate_));
    writer->writeTextElement(QStringLiteral("minbitrate"), QString::number(video_min_bit_rate_));
    writer->writeTextElement(QStringLiteral("maxbitrate"), QString::number(video_max_bit_rate_));
    writer->writeTextElement(QStringLiteral("bufsize"), QString::number(video_buffer_size_));
    writer->writeTextElement(QStringLiteral("threads"), QString::number(video_threads_));
    writer->writeTextElement(QStringLiteral("pixfmt"), video_pix_fmt_);
    writer->writeTextElement(QStringLiteral("imgseq"), QString::number(video_is_image_sequence_));

    writer->writeStartElement(QStringLiteral("color"));
      writer->writeTextElement(QStringLiteral("output"), color_transform_.output());
    writer->writeEndElement(); // colortransform

    writer->writeTextElement(QStringLiteral("vscale"), QString::number(video_scaling_method_));

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
    writer->writeTextElement(QStringLiteral("format"), QString::fromStdString(audio_params_.format().to_string()));
    writer->writeTextElement(QStringLiteral("bitrate"), QString::number(audio_bit_rate_));
  }

  writer->writeStartElement(QStringLiteral("subtitles"));

  writer->writeAttribute(QStringLiteral("enabled"), QString::number(subtitles_enabled_));

  if (subtitles_enabled_) {
    writer->writeTextElement(QStringLiteral("sidecar"), QString::number(subtitles_are_sidecar_));
    writer->writeTextElement(QStringLiteral("sidecarformat"), QString::number(subtitle_sidecar_fmt_));

    writer->writeTextElement(QStringLiteral("codec"), QString::number(subtitles_codec_));
  }

  writer->writeEndElement(); // subtitles

  writer->writeEndElement(); // audio

  writer->writeEndElement(); // export

  writer->writeEndDocument();
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

Encoder *Encoder::CreateFromParams(const EncodingParams &params)
{
  return CreateFromFormat(params.format(), params);
}

QStringList Encoder::GetPixelFormatsForCodec(ExportCodec::Codec c) const
{
  return QStringList();
}

std::vector<SampleFormat> Encoder::GetSampleFormatsForCodec(ExportCodec::Codec c) const
{
  return std::vector<SampleFormat>();
}

QMatrix4x4 EncodingParams::GenerateMatrix(EncodingParams::VideoScalingMethod method,
                                          int source_width, int source_height,
                                          int dest_width, int dest_height)
{
  QMatrix4x4 preview_matrix;

  if (method == EncodingParams::kStretch) {
    return preview_matrix;
  }

  float export_ar = static_cast<float>(dest_width) / static_cast<float>(dest_height);
  float source_ar = static_cast<float>(source_width) / static_cast<float>(source_height);

  if (qFuzzyCompare(export_ar, source_ar)) {
    return preview_matrix;
  }

  if ((export_ar > source_ar) == (method == EncodingParams::kFit)) {
    preview_matrix.scale(source_ar / export_ar, 1.0F);
  } else {
    preview_matrix.scale(1.0F, export_ar / source_ar);
  }

  return preview_matrix;
}

bool EncodingParams::LoadV1(QXmlStreamReader *reader)
{
  rational custom_range_in, custom_range_out;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("filename")) {
      filename_ = reader->readElementText();
    } else if (reader->name() == QStringLiteral("format")) {
      format_ = static_cast<ExportFormat::Format>(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("range")) {
      has_custom_range_ = reader->readElementText().toInt();
    } else if (reader->name() == QStringLiteral("customrangein")) {
      custom_range_in = rational::fromString(reader->readElementText().toStdString());
    } else if (reader->name() == QStringLiteral("customrangeout")) {
      custom_range_out = rational::fromString(reader->readElementText().toStdString());
    } else if (reader->name() == QStringLiteral("video")) {
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("enabled")) {
          video_enabled_ = attr.value().toInt();
        }
      }

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("codec")) {
          video_codec_ = static_cast<ExportCodec::Codec>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("width")) {
          video_params_.set_width(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("height")) {
          video_params_.set_height(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("format")) {
          video_params_.set_format(static_cast<PixelFormat::Format>(reader->readElementText().toInt()));
        } else if (reader->name() == QStringLiteral("pixelaspect")) {
          video_params_.set_pixel_aspect_ratio(rational::fromString(reader->readElementText().toStdString()));
        } else if (reader->name() == QStringLiteral("timebase")) {
          video_params_.set_time_base(rational::fromString(reader->readElementText().toStdString()));
        } else if (reader->name() == QStringLiteral("divider")) {
          video_params_.set_divider(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("bitrate")) {
          video_bit_rate_ = reader->readElementText().toLongLong();
        } else if (reader->name() == QStringLiteral("minbitrate")) {
          video_min_bit_rate_ = reader->readElementText().toLongLong();
        } else if (reader->name() == QStringLiteral("maxbitrate")) {
          video_max_bit_rate_ = reader->readElementText().toLongLong();
        } else if (reader->name() == QStringLiteral("bufsize")) {
          video_buffer_size_ = reader->readElementText().toLongLong();
        } else if (reader->name() == QStringLiteral("threads")) {
          video_threads_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("pixfmt")) {
          video_pix_fmt_ = reader->readElementText();
        } else if (reader->name() == QStringLiteral("imgseq")) {
          video_is_image_sequence_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("color")) {
          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("output")) {
              color_transform_ = reader->readElementText();
            } else {
              reader->skipCurrentElement();
            }
          }
        } else if (reader->name() == QStringLiteral("vscale")) {
          video_scaling_method_ = static_cast<VideoScalingMethod>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("opts")) {
          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("entry")) {
              QString key, value;
              while (XMLReadNextStartElement(reader)) {
                if (reader->name() == QStringLiteral("key")) {
                  key = reader->readElementText();
                } else if (reader->name() == QStringLiteral("value")) {
                  value = reader->readElementText();
                } else {
                  reader->skipCurrentElement();
                }
              }
              set_video_option(key, value);
            } else {
              reader->skipCurrentElement();
            }
          }
        } else {
          reader->skipCurrentElement();
        }
      }

      // HACK: Resolve bug where I forgot to serialize pixel aspect ratio
      if (video_params_.pixel_aspect_ratio().isNull()) {
        video_params_.set_pixel_aspect_ratio(1);
      }
    } else if (reader->name() == QStringLiteral("audio")) {
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("enabled")) {
          audio_enabled_ = attr.value().toInt();
        }
      }

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("codec")) {
          audio_codec_ = static_cast<ExportCodec::Codec>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("samplerate")) {
          audio_params_.set_sample_rate(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("channellayout")) {
          audio_params_.set_channel_layout(reader->readElementText().toULongLong());
        } else if (reader->name() == QStringLiteral("format")) {
          audio_params_.set_format(SampleFormat::from_string(reader->readElementText().toStdString()));
        } else if (reader->name() == QStringLiteral("bitrate")) {
          audio_bit_rate_ = reader->readElementText().toLongLong();
        } else {
          reader->skipCurrentElement();
        }
      }

      // HACK: Resolve bug where I forgot to serialize the audio bit rate
      if (!audio_bit_rate_) {
        audio_bit_rate_ = 320000;
      }
    } else if (reader->name() == QStringLiteral("subtitles")) {
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("enabled")) {
          subtitles_enabled_ = attr.value().toInt();
        }
      }

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("sidecar")) {
          subtitles_are_sidecar_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("sidecarformat")) {
          subtitle_sidecar_fmt_ = static_cast<ExportFormat::Format>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("codec")) {
          subtitles_codec_ = static_cast<ExportCodec::Codec>(reader->readElementText().toInt());
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  return true;
}

}
