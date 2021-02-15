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
  stream_count_(0),
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
    StreamReference ref = GetReferenceFromRealIndex(it.key());

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

      MetadataCache footage_info;

      if (QFileInfo::exists(meta_cache_file)) {

        // Load meta cache file
        footage_info = LoadStreamCache(meta_cache_file);

      } else {

        // Probe and create cache
        QVector<DecoderPtr> decoder_list = Decoder::ReceiveListOfAllDecoders();

        foreach (DecoderPtr decoder, decoder_list) {
          footage_info.streams = decoder->Probe(filename(), cancelled_);

          if (!footage_info.streams.isEmpty()) {
            footage_info.decoder = decoder->id();
            SetValid();
            break;
          }
        }

        if (!SaveStreamCache(meta_cache_file, footage_info)) {
          qWarning() << "Failed to save stream cache, footage will have to be re-probed";
        }

      }

      stream_count_ = footage_info.streams.size();

      if (!footage_info.streams.isEmpty()) {
        set_decoder(footage_info.decoder);

        for (int i=0; i<footage_info.streams.size(); i++) {
          const Stream& s = footage_info.streams.at(i);

          if (s.type() == Stream::kVideo || s.type() == Stream::kAudio) {
            QString id = GetInputIDOfIndex(i);

            NodeValue::Type type;

            if (s.type() == Stream::kVideo) {
              type = NodeValue::kVideoStreamProperties;
            } else {
              type = NodeValue::kAudioStreamProperties;
            }

            // Create input for properties
            AddInput(id, type, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
            SetStandardValue(id, QVariant::fromValue(s));
            inputs_for_stream_properties_.insert(i, id);

            // Create output for stream
            Footage::StreamReference ref = GetReferenceFromRealIndex(i);
            QString out_id = GetStringFromReference(ref);
            AddOutput(out_id);
            outputs_for_streams_.insert(i, out_id);
          }
        }

        SetValid();
      }

    } else {
      set_timestamp(0);
    }
  }
}

QString Footage::DescribeStream(int index) const
{
  Stream stream = GetStreamAt(index);

  switch (stream.type()) {
  case Stream::kVideo:
    if (stream.video_type() == Stream::kVideoTypeStill) {
      return tr("%1: Image - %2x%3").arg(QString::number(index),
                                         QString::number(stream.width()),
                                         QString::number(stream.height()));
    } else {
      return tr("%1: Video - %2x%3").arg(QString::number(index),
                                         QString::number(stream.width()),
                                         QString::number(stream.height()));
    }
  case Stream::kAudio:
    return QCoreApplication::translate("Stream", "%1: Audio - %2 Channel(s), %3Hz")
        .arg(QString::number(index),
             QString::number(stream.channel_count()),
             QString::number(stream.sample_rate()));
  case Stream::kUnknown:
  case Stream::kData:
  case Stream::kSubtitle:
  case Stream::kAttachment:
    break;
  }

  return tr("%1: Unknown").arg(QString::number(index));
}

Footage::StreamReference Footage::GetReferenceFromOutput(const QString &s) const
{
  Stream::Type type = GetTypeFromOutput(s);

  if (type != Stream::kUnknown) {
    bool ok;
    int index = s.mid(2).toInt(&ok);

    if (ok) {
      return StreamReference(this, type, index);
    }
  }

  return StreamReference();
}

int Footage::GetStreamTypeCount(Stream::Type type) const
{
  int count = 0;

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    Stream s = GetStreamAt(it.key());

    if (s.type() == type) {
      count++;
    }
  }

  return count;
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

  // Reset stream count
  stream_count_ = 0;

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

int64_t Footage::GetTimeInTimebaseUnits(int index, const rational &time) const
{
  Stream s = GetStreamAt(index);

  if (!s.IsValid()) {
    return AV_NOPTS_VALUE;
  }

  return Timecode::time_to_timestamp(time, s.timebase()) + s.start_time();
}

int Footage::GetRealStreamIndex(Stream::Type type, int index) const
{
  int lookup_index = 0;

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    Stream stream = GetStandardValue(it.value()).value<Stream>();

    if (stream.type() == type) {
      if (lookup_index == index) {
        return it.key();
      } else {
        lookup_index++;
      }
    }
  }

  return -1;
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

Footage::StreamReference Footage::GetReferenceFromRealIndex(int real_index) const
{
  Stream s = GetStreamAt(real_index);

  if (!s.IsValid()) {
    // Return invalid/null reference
    return StreamReference();
  }

  int index_in_type = 0;
  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    if (it.key() == real_index) {
      break;
    } else {
      Stream temp = GetStreamAt(it.key());

      if (temp.type() == s.type()) {
        index_in_type++;
      }
    }
  }

  return StreamReference(this, s.type(), index_in_type);
}

Stream::Type Footage::GetTypeFromOutput(const QString &s) const
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

Stream Footage::GetFirstEnabledStreamOfType(Stream::Type type) const
{
  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    Stream stream = GetStandardValue(it.value()).value<Stream>();

    if (stream.enabled() && stream.type() == type) {
      return stream;
    }
  }

  return Stream();
}

QVector<int> Footage::GetStreamIndexesOfType(Stream::Type type) const
{
  QVector<int> indexes;

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    Stream stream = GetStandardValue(it.value()).value<Stream>();

    if (stream.enabled() && stream.type() == type) {
      indexes.append(it.key());
    }
  }

  return indexes;
}

const QString &Footage::decoder() const
{
  return decoder_;
}

void Footage::set_decoder(const QString &id)
{
  decoder_ = id;
}

QIcon Footage::icon() const
{
  if (valid_ && !inputs_for_stream_properties_.isEmpty()) {
    // Prioritize video > audio > image
    Stream s = GetFirstEnabledStreamOfType(Stream::kVideo);

    if (s.IsValid() && s.video_type() != Stream::kVideoTypeStill) {
      return icon::Video;
    } else if (HasEnabledStreamsOfType(Stream::kAudio)) {
      return icon::Audio;
    } else if (s.IsValid() && s.video_type() == Stream::kVideoTypeStill) {
      return icon::Image;
    }
  }

  return icon::Error;
}

QString Footage::duration()
{
  // Find longest stream duration
  Stream longest_stream;
  rational longest;

  for (auto it=inputs_for_stream_properties_.cbegin(); it!=inputs_for_stream_properties_.cend(); it++) {
    Stream s = GetStandardValue(it.value()).value<Stream>();

    if (s.enabled() && (s.type() == Stream::kVideo || s.type() == Stream::kAudio)) {
      rational this_stream_dur = Timecode::timestamp_to_time(s.duration(), s.timebase());

      if (this_stream_dur > longest) {
        longest_stream = s;
        longest = this_stream_dur;
      }
    }
  }

  if (longest_stream.IsValid()) {
    if (longest_stream.type() == Stream::kVideo) {
      if (longest_stream.video_type() != Stream::kVideoTypeStill) {
        int64_t duration = longest_stream.duration();
        rational frame_rate_timebase = longest_stream.frame_rate().flipped();

        if (longest_stream.timebase() != frame_rate_timebase) {
          // Convert from timebase to frame rate
          rational duration_time = Timecode::timestamp_to_time(duration, longest_stream.timebase());
          duration = Timecode::time_to_timestamp(duration_time, frame_rate_timebase);
        }

        return Timecode::timestamp_to_timecode(duration,
                                               frame_rate_timebase,
                                               Core::instance()->GetTimecodeDisplay());
      }
    } else if (longest_stream.type() == Stream::kAudio) {
      // If we're showing in a timecode, we prefer showing audio in seconds instead
      Timecode::Display display = Core::instance()->GetTimecodeDisplay();
      if (display == Timecode::kTimecodeDropFrame
          || display == Timecode::kTimecodeNonDropFrame) {
        display = Timecode::kTimecodeSeconds;
      }

      return Timecode::timestamp_to_timecode(longest_stream.duration(),
                                             longest_stream.timebase(),
                                             display);
    }
  }

  return QString();
}

QString Footage::rate()
{
  if (inputs_for_stream_properties_.isEmpty()) {
    return QString();
  }

  if (HasEnabledStreamsOfType(Stream::kVideo)) {
    // This is a video editor, prioritize video streams
    Stream video_stream = GetFirstEnabledStreamOfType(Stream::kVideo);

    if (video_stream.video_type() != Stream::kVideoTypeStill) {
      return QCoreApplication::translate("Footage", "%1 FPS").arg(video_stream.frame_rate().toDouble());
    }
  } else if (HasEnabledStreamsOfType(Stream::kAudio)) {
    // No video streams, return audio
    Stream audio_stream = GetStreamAt(0);
    return QCoreApplication::translate("Footage", "%1 Hz").arg(audio_stream.sample_rate());
  }

  return QString();
}

quint64 Footage::get_enabled_stream_flags() const
{
  quint64 enabled_streams = 0;

  for (int i=0; i<stream_count_; i++) {
    if (IsStreamEnabled(i)) {
      enabled_streams |= (1 << i);
    }
  }

  return enabled_streams;
}

bool Footage::HasEnabledStreamsOfType(const Stream::Type &type) const
{
  // Return true if any streams are video streams
  for (int i=0; i<stream_count_; i++) {
    Stream s = GetStreamAt(i);

    if (s.enabled() && s.type() == type) {
      return true;
    }
  }

  return false;
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
    Stream stream = GetStreamAt(GetReferenceFromOutput(output));

    if (stream.IsValid()) {
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
        hash.addData(stream.colorspace().toUtf8());

        // Alpha associated setting
        hash.addData(QString::number(stream.premultiplied_alpha()).toUtf8());

        // Pixel aspect ratio
        hash.addData(reinterpret_cast<const char*>(&stream.pixel_aspect_ratio()), sizeof(stream.pixel_aspect_ratio()));

        // Footage timestamp
        if (stream.video_type() != Stream::kVideoTypeStill) {
          int64_t video_ts = Timecode::time_to_timestamp(time, stream.timebase());

          // Add timestamp in units of the video stream's timebase
          hash.addData(reinterpret_cast<const char*>(&video_ts), sizeof(int64_t));

          // Add start time - used for both image sequences and video streams
          hash.addData(QString::number(stream.start_time()).toUtf8());
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
  if (QFileInfo(file).exists() && ref.IsValid()) {
    table.Push(NodeValue::kFootageJob, QVariant::fromValue(ref), this);
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

    if (!inputs_for_stream_properties_.isEmpty()) {
      for (int i=0; i<stream_count_; i++) {
        Stream s = GetStreamAt(i);

        if (s.enabled()) {
          tip.append("\n");
          tip.append(DescribeStream(i));
        }
      }
    }

    set_tooltip(tip);
  } else {
    set_tooltip(QCoreApplication::translate("Footage", "This footage is not valid for use"));
  }
}

Footage::MetadataCache Footage::LoadStreamCache(const QString &filename)
{
  MetadataCache cache;
  QFile file(filename);

  if (file.open(QFile::ReadOnly)) {
    QXmlStreamReader reader(&file);

    while (XMLReadNextStartElement(&reader)) {
      if (reader.name() == QStringLiteral("streamcache")) {
        while (XMLReadNextStartElement(&reader)) {
          if (reader.name() == QStringLiteral("decoder")) {
            cache.decoder = reader.readElementText();
          } else if (reader.name() == QStringLiteral("streams")) {
            while (XMLReadNextStartElement(&reader)) {
              if (reader.name() == QStringLiteral("stream")) {
                Stream s;
                s.Load(&reader);
                cache.streams.append(s);
              } else {
                reader.skipCurrentElement();
              }
            }
          } else {
            reader.skipCurrentElement();
          }
        }
      } else {
        reader.skipCurrentElement();
      }
    }

    file.close();
  }

  return cache;
}

bool Footage::SaveStreamCache(const QString &filename, const Footage::MetadataCache &data)
{
  QFile file(filename);

  if (!file.open(QFile::WriteOnly)) {
    return false;
  }

  QXmlStreamWriter writer(&file);

  writer.writeStartDocument();

  writer.writeStartElement(QStringLiteral("streamcache"));

  writer.writeTextElement(QStringLiteral("decoder"), data.decoder);

  writer.writeStartElement(QStringLiteral("streams"));

  foreach (const Stream& s, data.streams) {
    writer.writeStartElement(QStringLiteral("stream"));
    s.Save(&writer);
    writer.writeEndElement(); // stream
  }

  writer.writeEndElement(); // streams

  writer.writeEndElement(); // streamcache

  writer.writeEndDocument();

  file.close();

  return true;
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

QString Footage::StreamReference::video_colorspace(bool default_if_empty) const
{
  if (IsValid()) {
    Stream stream = footage_->GetStreamAt(type_, index_);

    if (stream.IsValid()) {
      if (stream.colorspace().isEmpty() && default_if_empty) {
        return footage_->project()->color_manager()->GetDefaultInputColorSpace();
      } else {
        return stream.colorspace();
      }
    }
  }

  return QString();
}

uint qHash(const Footage::StreamReference &ref, uint seed)
{
  return qHash(ref.footage(), seed) ^ qHash(ref.type(), seed) ^ qHash(ref.index(), seed);
}

}
