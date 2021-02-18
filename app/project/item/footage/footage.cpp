/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "footage.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include "codec/decoder.h"
#include "common/filefunctions.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "core.h"
#include "render/job/footagejob.h"
#include "ui/icons/icons.h"

namespace olive {

const QString Footage::kFilenameInput = QStringLiteral("file_in");
const QString Footage::kStreamPropertiesFormat = QStringLiteral("stream_properties:%1");

#define super Item

Footage::Footage(const QString &filename) :
  super(true, false),
  cancelled_(nullptr)
{
  AddInput(kFilenameInput, NodeValue::kFile, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  Clear();

  set_filename(filename);
}

void Footage::Retranslate()
{
  super::Retranslate();

  SetInputName(kFilenameInput, tr("Filename"));

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    StreamReference ref = it.key();

    SetInputName(it.value(), QStringLiteral("%1 %2").arg(GetStreamTypeName(ref.type()), QString::number(ref.index())));
  }
}

void Footage::LoadInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt* cancelled)
{
  Q_UNUSED(xml_node_data)
  Q_UNUSED(version)

  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("timestamp")) {
      set_timestamp(reader->readElementText().toLongLong());
    } else if (reader->name() == QStringLiteral("points")) {
      TimelinePoints::Load(reader);
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Footage::SaveInternal(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("timestamp"), QString::number(timestamp_));

  writer->writeStartElement(QStringLiteral("points"));
  TimelinePoints::Save(writer);
  writer->writeEndElement(); // points
}

void Footage::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kFilenameInput) {
    // Reset internal stream cache
    Clear();

    // Determine if file still exists
    QFileInfo info(filename());

    if (info.exists()) {
      // Grab timestamp
      set_timestamp(info.lastModified().toMSecsSinceEpoch());

      // Determine if we've already cached the metadata of this file
      QString meta_cache_file = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(FileFunctions::GetUniqueFileIdentifier(filename()));

      FootageDescription footage_info;

      if (QFileInfo::exists(meta_cache_file)) {

        // Load meta cache file
        footage_info.Load(meta_cache_file);

      } else {

        // Probe and create cache
        QVector<DecoderPtr> decoder_list = Decoder::ReceiveListOfAllDecoders();

        foreach (DecoderPtr decoder, decoder_list) {
          footage_info = decoder->Probe(filename(), cancelled_);

          if (footage_info.IsValid()) {
            break;
          }
        }

        if (!footage_info.Save(meta_cache_file)) {
          qWarning() << "Failed to save stream cache, footage will have to be re-probed";
        }

      }

      if (footage_info.IsValid()) {
        decoder_ = footage_info.decoder();

        for (int i=0; i<footage_info.GetVideoStreams().size(); i++) {
          AddStreamAsInput(Stream::kVideo, i, QVariant::fromValue(footage_info.GetVideoStreams().at(i)));
        }

        for (int i=0; i<footage_info.GetAudioStreams().size(); i++) {
          AddStreamAsInput(Stream::kAudio, i, QVariant::fromValue(footage_info.GetAudioStreams().at(i)));
        }

        SetValid();
      }

    } else {
      set_timestamp(0);
    }
  }
}

Footage::StreamReference Footage::GetReferenceFromOutput(const QString &s) const
{
  Stream::Type type;
  int index;

  if (GetReferenceFromOutput(s, &type, &index)) {
    return StreamReference(type, index);
  } else {
    return StreamReference();
  }
}

bool Footage::GetReferenceFromOutput(const QString &s, Stream::Type *type, int *index)
{
  Stream::Type parse_type = GetTypeFromOutput(s);

  if (parse_type != Stream::kUnknown) {
    bool ok;
    int parse_index = s.mid(2).toInt(&ok);

    if (ok) {
      *type = parse_type;
      *index = parse_index;
      return true;
    }
  }

  return false;
}

VideoParams Footage::GetFirstEnabledVideoStream() const
{
  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    if (it.key().type() == Stream::kVideo) {
      VideoParams vp = GetVideoParams(it.key().index());

      if (vp.enabled()) {
        return vp;
      }
    }
  }

  return VideoParams();
}

AudioParams Footage::GetFirstEnabledAudioStream() const
{
  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    if (it.key().type() == Stream::kAudio) {
      AudioParams ap = GetAudioParams(it.key().index());

      if (ap.enabled()) {
        return ap;
      }
    }
  }

  return AudioParams();
}

QVector<VideoParams> Footage::GetEnabledVideoStreams() const
{
  QVector<VideoParams> list;

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    if (it.key().type() == Stream::kVideo) {
      VideoParams vp = GetVideoParams(it.key().index());

      if (vp.enabled()) {
        list.append(vp);
      }
    }
  }

  return list;
}

QVector<AudioParams> Footage::GetEnabledAudioStreams() const
{
  QVector<AudioParams> list;

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    if (it.key().type() == Stream::kAudio) {
      AudioParams ap = GetAudioParams(it.key().index());

      if (ap.enabled()) {
        list.append(ap);
      }
    }
  }

  return list;
}

QVector<Footage::StreamReference> Footage::GetEnabledStreamsAsReferences() const
{
  QVector<Footage::StreamReference> refs;

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    refs.append(StreamReference(it.key().type(), it.key().index()));
  }

  return refs;
}

void Footage::Clear()
{
  // Clear all dynamically created inputs
  foreach (const QString& s, inputs_for_stream_properties_) {
    RemoveInput(s);
  }
  inputs_for_stream_properties_.clear();

  // Clear all dynamically created outputs
  foreach (const QString& s, outputs_for_streams_) {
    RemoveOutput(s);
  }
  outputs_for_streams_.clear();

  // Clear decoder link
  decoder_.clear();

  // Reset ready state
  valid_ = false;
}

void Footage::SetValid()
{
  valid_ = true;
}

QString Footage::filename() const
{
  return GetStandardValue(kFilenameInput).toString();
}

void Footage::set_filename(const QString &s)
{
  SetStandardValue(kFilenameInput, s);
}

const qint64 &Footage::timestamp() const
{
  return timestamp_;
}

void Footage::set_timestamp(const qint64 &t)
{
  timestamp_ = t;
}

QString Footage::GetStringFromReference(Stream::Type type, int index)
{
  QString type_string;

  if (type == Stream::kVideo) {
    type_string = QStringLiteral("v");
  } else if (type == Stream::kAudio) {
    type_string = QStringLiteral("a");
  } else {
    return QString();
  }

  return QStringLiteral("%1:%2").arg(type_string, QString::number(index));
}

int Footage::GetStreamIndex(Stream::Type type, int index) const
{
  if (type == Stream::kVideo) {
    return GetVideoParams(index).stream_index();
  } else if (type == Stream::kAudio) {
    return GetAudioParams(index).stream_index();
  } else {
    return -1;
  }
}

Stream::Type Footage::GetTypeFromOutput(const QString &s)
{
  if (s.at(1) == ':') {
    if (s.at(0) == 'v') {
      // Video stream
      return Stream::kVideo;
    } else if (s.at(0) == 'a') {
      // Audio stream
      return Stream::kAudio;
    }
  }

  return Stream::kUnknown;
}

const QString &Footage::decoder() const
{
  return decoder_;
}

QIcon Footage::icon() const
{
  if (valid_ && !inputs_for_stream_properties_.isEmpty()) {
    // Prioritize video > audio > image
    VideoParams s = GetFirstEnabledVideoStream();

    if (s.is_valid() && s.video_type() != VideoParams::kVideoTypeStill) {
      return icon::Video;
    } else if (HasEnabledAudioStreams()) {
      return icon::Audio;
    } else if (s.is_valid() && s.video_type() == VideoParams::kVideoTypeStill) {
      return icon::Image;
    }
  }

  return icon::Error;
}

QString Footage::duration()
{
  // Try video first
  VideoParams video = GetFirstEnabledVideoStream();

  if (video.is_valid() && video.video_type() != VideoParams::kVideoTypeStill) {
    int64_t duration = video.duration();
    rational frame_rate_timebase = video.frame_rate().flipped();

    if (video.time_base() != frame_rate_timebase) {
      // Convert from timebase to frame rate
      duration = Timecode::rescale_timestamp_ceil(duration, video.time_base(), frame_rate_timebase);
    }

    return Timecode::timestamp_to_timecode(duration,
                                           frame_rate_timebase,
                                           Core::instance()->GetTimecodeDisplay());
  }

  // Try audio second
  AudioParams audio = GetFirstEnabledAudioStream();

  if (audio.is_valid()) {
    // If we're showing in a timecode, we prefer showing audio in seconds instead
    Timecode::Display display = Core::instance()->GetTimecodeDisplay();
    if (display == Timecode::kTimecodeDropFrame
        || display == Timecode::kTimecodeNonDropFrame) {
      display = Timecode::kTimecodeSeconds;
    }

    return Timecode::timestamp_to_timecode(audio.duration(),
                                           audio.time_base(),
                                           display);
  }

  // Otherwise, return nothing
  return QString();
}

QString Footage::rate()
{
  if (inputs_for_stream_properties_.isEmpty()) {
    return QString();
  }

  if (HasEnabledVideoStreams()) {
    // This is a video editor, prioritize video streams
    VideoParams video_stream = GetFirstEnabledVideoStream();

    if (video_stream.video_type() != VideoParams::kVideoTypeStill) {
      return QCoreApplication::translate("Footage", "%1 FPS").arg(video_stream.frame_rate().toDouble());
    }
  } else if (HasEnabledAudioStreams()) {
    // No video streams, return audio
    AudioParams audio_stream = GetFirstEnabledAudioStream();
    return QCoreApplication::translate("Footage", "%1 Hz").arg(audio_stream.sample_rate());
  }

  return QString();
}

bool Footage::HasEnabledVideoStreams() const
{
  return GetFirstEnabledVideoStream().is_valid();
}

bool Footage::HasEnabledAudioStreams() const
{
  return GetFirstEnabledAudioStream().is_valid();
}

QString Footage::DescribeVideoStream(const VideoParams &params)
{
  if (params.video_type() == VideoParams::kVideoTypeStill) {
    return tr("%1: Image - %2x%3").arg(QString::number(params.stream_index()),
                                       QString::number(params.width()),
                                       QString::number(params.height()));
  } else {
    return tr("%1: Video - %2x%3").arg(QString::number(params.stream_index()),
                                       QString::number(params.width()),
                                       QString::number(params.height()));
  }
}

QString Footage::DescribeAudioStream(const AudioParams &params)
{
  return tr("%1: Audio - %2 Channel(s), %3Hz")
      .arg(QString::number(params.stream_index()),
           QString::number(params.channel_count()),
           QString::number(params.sample_rate()));
}

bool Footage::CompareFootageToFile(Footage *footage, const QString &filename)
{
  // Heuristic to determine if file has changed
  QFileInfo info(filename);

  if (info.exists()) {
    /*if (info.lastModified().toMSecsSinceEpoch() == footage->timestamp()) {
      // Footage has not been modified and is where we expect
      return true;
    } else {
      // Footage may have changed and we'll have to re-probe it. It also may not have, in which
      // case nothing needs to change.
      DecoderPtr decoder = Decoder::CreateFromID(footage->decoder());

      Streams probed_streams = decoder->Probe(filename, nullptr);

      if (probed_streams == footage->streams_) {
        return true;
      }
    }*/
    Q_UNUSED(footage)

    // Simplified, since our footage node is much more tolerant, we'll try this
    return true;
  }

  // Footage file couldn't be found or resolved to something we didn't expect
  return false;
}

bool Footage::CompareFootageToItsFilename(Footage *footage)
{
  return CompareFootageToFile(footage, footage->filename());
}

void Footage::Hash(const QString& output, QCryptographicHash &hash, const rational &time) const
{
  super::Hash(output, hash, time);

  // Translate output ID to stream
  StreamReference ref = GetReferenceFromOutput(output);

  QString fn = filename();

  if (!fn.isEmpty()) {
    VideoParams params = GetVideoParams(ref.index());

    if (params.is_valid()) {
      // Add footage details to hash

      // Footage filename
      hash.addData(filename().toUtf8());

      // Footage last modified date
      hash.addData(QString::number(timestamp()).toUtf8());

      // Footage stream
      hash.addData(QString::number(ref.index()).toUtf8());

      if (ref.type() == Stream::kVideo) {
        // Current color config and space
        hash.addData(project()->color_manager()->GetConfigFilename().toUtf8());
        hash.addData(params.colorspace().toUtf8());

        // Alpha associated setting
        hash.addData(QString::number(params.premultiplied_alpha()).toUtf8());

        // Pixel aspect ratio
        hash.addData(reinterpret_cast<const char*>(&params.pixel_aspect_ratio()), sizeof(params.pixel_aspect_ratio()));

        // Footage timestamp
        if (params.video_type() != VideoParams::kVideoTypeStill) {
          int64_t video_ts = Timecode::time_to_timestamp(time, params.time_base());

          // Add timestamp in units of the video stream's timebase
          hash.addData(reinterpret_cast<const char*>(&video_ts), sizeof(int64_t));

          // Add start time - used for both image sequences and video streams
          hash.addData(QString::number(params.start_time()).toUtf8());
        }
      }
    }
  }
}

NodeValueTable Footage::Value(const QString &output, NodeValueDatabase &value) const
{
  StreamReference ref = GetReferenceFromOutput(output);

  // Pop filename from table
  QString file = value[kFilenameInput].Take(NodeValue::kFile).toString();

  // Merge table
  NodeValueTable table = value.Merge();

  // If the file exists and the reference is valid, push a footage job to the renderer
  if (QFileInfo(file).exists()) {
    FootageJob job(decoder_, filename(), ref.type());

    if (ref.type() == Stream::kVideo) {
      VideoParams vp = GetVideoParams(ref.index());

      if (vp.colorspace().isEmpty()) {
        vp.set_colorspace(project()->color_manager()->GetDefaultInputColorSpace());
      }

      job.set_video_params(vp);
    } else {
      job.set_audio_params(GetAudioParams(ref.index()));
      job.set_cache_path(project()->cache_path());
    }

    table.Push(NodeValue::kFootageJob, QVariant::fromValue(job), this);
  }

  return table;
}

QString Footage::GetStreamTypeName(Stream::Type type)
{
  switch (type) {
  case Stream::kVideo:
    return tr("Video");
  case Stream::kAudio:
    return tr("Audio");
  case Stream::kSubtitle:
    return tr("Subtitle");
  case Stream::kData:
    return tr("Data");
  case Stream::kAttachment:
    return tr("Attachment");
  case Stream::kUnknown:
    break;
  }

  return tr("Unknown");
}

void Footage::UpdateTooltip()
{
  if (valid_) {
    QString tip = QCoreApplication::translate("Footage", "Filename: %1").arg(filename());

    for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
      if (it.key().type() == Stream::kVideo) {
        VideoParams p = GetVideoParams(it.key().index());

        if (p.enabled()) {
          tip.append("\n");
          tip.append(DescribeVideoStream(p));
        }
      } else if (it.key().type() == Stream::kAudio) {
        AudioParams p = GetAudioParams(it.key().index());

        if (p.enabled()) {
          tip.append("\n");
          tip.append(DescribeAudioStream(p));
        }
      }
    }

    set_tooltip(tip);
  } else {
    set_tooltip(QCoreApplication::translate("Footage", "This footage is not valid for use"));
  }
}

void Footage::AddStreamAsInput(Stream::Type type, int index, QVariant value)
{
  QString input_id = GetInputIDOfIndex(type, index);

  StreamReference ref(type, index);

  // Create input for parameters
  AddInput(input_id, type == Stream::kVideo ? NodeValue::kVideoParams : NodeValue::kAudioParams,
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  SetStandardValue(input_id, value);
  inputs_for_stream_properties_.insert(ref, input_id);

  // Create output for stream
  QString output_id = GetStringFromReference(type, index);
  AddOutput(output_id);
  outputs_for_streams_.insert(ref, output_id);
}

void Footage::CheckFootage()
{
  QString fn = filename();

  if (!fn.isEmpty()) {
    QFileInfo info(fn);

    qint64 current_file_timestamp = info.lastModified().toMSecsSinceEpoch();

    if (current_file_timestamp != timestamp()) {
      // File has changed!
      set_timestamp(current_file_timestamp);
      InvalidateAll(kFilenameInput);
    }
  }
}

/*QString Footage::StreamReference::video_colorspace(bool default_if_empty) const
{
  if (IsValid()) {
    VideoParams params = footage_->GetVideoParams(index_);

    if (params.is_valid()) {
      if (params.colorspace().isEmpty() && default_if_empty) {

      } else {
        return params.colorspace();
      }
    }
  }

  return QString();
}*/

uint qHash(const Footage::StreamReference &ref, uint seed)
{
  return qHash(ref.type(), seed) ^ qHash(ref.index(), seed);
}

QDataStream &operator<<(QDataStream &out, const Footage::StreamReference &ref)
{
  out << static_cast<int>(ref.type()) << ref.index();

  return out;
}

QDataStream &operator>>(QDataStream &in, Footage::StreamReference &ref)
{
  int type;
  int index;

  in >> type >> index;

  ref = Footage::StreamReference(static_cast<Stream::Type>(type), index);

  return in;
}

}
