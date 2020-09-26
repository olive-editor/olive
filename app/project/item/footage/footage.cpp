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

#include "footage.h"

#include <QCoreApplication>
#include <QDir>

#include "codec/decoder.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "core.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

Footage::Footage()
{
  Clear();
}

Footage::~Footage()
{
  ClearStreams();
}

void Footage::Load(QXmlStreamReader *reader, XMLNodeData &xml_node_data, const QAtomicInt* cancelled)
{
  /*
  QXmlStreamAttributes attributes = reader->attributes();

  foreach (const QXmlStreamAttribute& attr, attributes) {
    if (attr.name() == QStringLiteral("name")) {
      set_name(attr.value().toString());
    } else if (attr.name() == QStringLiteral("filename")) {
      set_filename(attr.value().toString());
    }
  }

  // Validate filename
  if (!QFileInfo::exists(filename_)) {
    // Absolute filename does not exist, use some heuristics to try relocating the file

    if (xml_node_data.real_project_url != xml_node_data.saved_project_url) {
      // Project path has changed, check if the file we're looking for is the same relative to the
      // new project path
      QDir saved_dir(QFileInfo(xml_node_data.saved_project_url).dir());
      QDir true_dir(QFileInfo(xml_node_data.real_project_url).dir());

      QString relative_filename = saved_dir.relativeFilePath(filename_);
      QString transformed_abs_filename = true_dir.filePath(relative_filename);

      if (QFileInfo::exists(transformed_abs_filename)) {
        // Use this file instead
        qInfo() << "Footage" << filename_ << "doesn't exist, using relative file" << transformed_abs_filename;
        set_filename(transformed_abs_filename);
      }
    }
  }

  Decoder::ProbeMedia(this, cancelled);

  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("stream")) {
      int stream_index = -1;
      quintptr stream_ptr = 0;

      XMLAttributeLoop(reader, attr) {
        if (cancelled && *cancelled) {
          return;
        }

        if (attr.name() == QStringLiteral("index")) {
          stream_index = attr.value().toInt();
        } else if (attr.name() == QStringLiteral("ptr")) {
          stream_ptr = attr.value().toULongLong();
        }
      }

      if (stream_index > -1 && stream_ptr > 0) {
        xml_node_data.footage_ptrs.insert(stream_ptr, stream(stream_index));

        stream(stream_index)->Load(reader);
      } else {
        qWarning() << "Invalid stream found in project file";
      }
    } else if (reader->name() == QStringLiteral("points")) {
      TimelinePoints::Load(reader);
    } else {
      reader->skipCurrentElement();
    }
  }
  */
}

void Footage::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("footage");

  writer->writeAttribute("name", name());
  writer->writeAttribute("filename", filename());

  TimelinePoints::Save(writer);

  foreach (StreamPtr stream, streams_) {
    stream->Save(writer);
  }

  writer->writeEndElement(); // footage
}

void Footage::Clear()
{
  // Clear all streams
  ClearStreams();

  // Reset ready state
  valid_ = false;
}

void Footage::SetValid()
{
  valid_ = true;
}

const QString &Footage::filename() const
{
  return filename_;
}

void Footage::set_filename(const QString &s)
{
  filename_ = s;
}

const QDateTime &Footage::timestamp() const
{
  return timestamp_;
}

void Footage::set_timestamp(const QDateTime &t)
{
  timestamp_ = t;
}

void Footage::add_stream(StreamPtr s)
{
  // Set its footage parent to this
  s->set_footage(this);

  // Add a copy of this stream to the list
  streams_.append(s);
}

StreamPtr Footage::stream(int index) const
{
  return streams_.at(index);
}

const QList<StreamPtr> &Footage::streams() const
{
  return streams_;
}

int Footage::stream_count() const
{
  return streams_.size();
}

Item::Type Footage::type() const
{
  return kFootage;
}

const QString &Footage::decoder() const
{
  return decoder_;
}

void Footage::set_decoder(const QString &id)
{
  decoder_ = id;
}

QIcon Footage::icon()
{
  if (valid_ && !streams_.isEmpty()) {
    StreamPtr first_stream = streams_.first();

    if (first_stream->type() == Stream::kVideo) {
      if (std::static_pointer_cast<VideoStream>(first_stream)->video_type() == VideoStream::kVideoTypeStill) {
        return icon::Image;
      } else {
        return icon::Video;
      }
    } else if (first_stream->type() == Stream::kAudio) {
      return icon::Audio;
    }
  }

  return icon::Error;
}

QString Footage::duration()
{
  // Find longest stream duration

  StreamPtr longest_stream = nullptr;
  rational longest;

  foreach (StreamPtr stream, streams_) {
    if (stream->enabled() && (stream->type() == Stream::kVideo || stream->type() == Stream::kAudio)) {
      rational this_stream_dur = Timecode::timestamp_to_time(stream->duration(),
                                                             stream->timebase());

      if (this_stream_dur > longest) {
        longest_stream = stream;
        longest = this_stream_dur;
      }
    }
  }

  if (longest_stream) {
    if (longest_stream->type() == Stream::kVideo) {
      VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(longest_stream);

      int64_t duration = video_stream->duration();
      rational frame_rate_timebase = video_stream->frame_rate().flipped();

      if (video_stream->timebase() != frame_rate_timebase) {
        // Convert from timebase to frame rate
        rational duration_time = Timecode::timestamp_to_time(duration, video_stream->timebase());
        duration = Timecode::time_to_timestamp(duration_time, frame_rate_timebase);
      }

      return Timecode::timestamp_to_timecode(duration,
                                             frame_rate_timebase,
                                             Core::instance()->GetTimecodeDisplay());
    } else if (longest_stream->type() == Stream::kAudio) {
      AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(longest_stream);

      // If we're showing in a timecode, we prefer showing audio in seconds instead
      Timecode::Display display = Core::instance()->GetTimecodeDisplay();
      if (display == Timecode::kTimecodeDropFrame
          || display == Timecode::kTimecodeNonDropFrame) {
        display = Timecode::kTimecodeSeconds;
      }

      return Timecode::timestamp_to_timecode(longest_stream->duration(),
                                             longest_stream->timebase(),
                                             display);
    }
  }

  return QString();
}

QString Footage::rate()
{
  if (streams_.isEmpty()) {
    return QString();
  }

  if (HasStreamsOfType(Stream::kVideo)
      && std::static_pointer_cast<VideoStream>(get_first_stream_of_type(Stream::kVideo))->video_type() != VideoStream::kVideoTypeStill) {
    // This is a video editor, prioritize video streams
    VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(get_first_stream_of_type(Stream::kVideo));
    return QCoreApplication::translate("Footage", "%1 FPS").arg(video_stream->frame_rate().toDouble());
  } else if (HasStreamsOfType(Stream::kAudio)) {
    // No video streams, return audio
    AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(streams_.first());
    return QCoreApplication::translate("Footage", "%1 Hz").arg(audio_stream->sample_rate());
  }

  return QString();
}

quint64 Footage::get_enabled_stream_flags() const
{
  quint64 enabled_streams = 0;
  quint64 stream_enabler = 1;

  foreach (StreamPtr s, streams_) {
    if (s->enabled()) {
      enabled_streams |= stream_enabler;
    }

    stream_enabler <<= 1;
  }

  return enabled_streams;
}

void Footage::ClearStreams()
{
  // Delete all streams
  streams_.clear();
}

bool Footage::HasStreamsOfType(const Stream::Type &type) const
{
  // Return true if any streams are video streams
  foreach (StreamPtr stream, streams_) {
    if (stream->enabled() && stream->type() == type) {
      return true;
    }
  }

  return false;
}

StreamPtr Footage::get_first_stream_of_type(const Stream::Type &type) const
{
  foreach (StreamPtr stream, streams_) {
    if (stream->enabled() && stream->type() == type) {
      return stream;
    }
  }

  return nullptr;
}

void Footage::UpdateTooltip()
{
  if (valid_) {
    QString tip = QCoreApplication::translate("Footage", "Filename: %1").arg(filename());

    if (!streams_.isEmpty()) {
      foreach (StreamPtr s, streams_) {
        if (s->enabled()) {
          tip.append("\n");
          tip.append(s->description());
        }
      }
    }

    set_tooltip(tip);
  } else {
    set_tooltip(QCoreApplication::translate("Footage", "This footage is not valid for use"));
  }
}

OLIVE_NAMESPACE_EXIT
