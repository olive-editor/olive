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
  QXmlStreamAttributes attributes = reader->attributes();

  foreach (const QXmlStreamAttribute& attr, attributes) {
    if (attr.name() == QStringLiteral("name")) {
      set_name(attr.value().toString());
    } else if (attr.name() == QStringLiteral("filename")) {
      set_filename(attr.value().toString());
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

const Footage::Status& Footage::status() const
{
  return status_;
}

void Footage::set_status(const Footage::Status &status)
{
  status_ = status;

  UpdateTooltip();
}

void Footage::Clear()
{
  // Clear all streams
  ClearStreams();

  // Reset ready state
  set_status(kUnprobed);
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
  switch (status_) {
  case kUnprobed:
  case kUnindexed:
    // FIXME Set a waiting icon
    return QIcon();
  case kReady:
    if (HasStreamsOfType(Stream::kVideo)) {

      // Prioritize the video icon
      return icon::Video;

    } else if (HasStreamsOfType(Stream::kAudio)) {

      // Otherwise assume it's audio only
      return icon::Audio;

    } else if (HasStreamsOfType(Stream::kImage)) {

      // Otherwise assume it's an image
      return icon::Image;

    }
    /* fall-through */
  case kInvalid:
    return icon::Error;
  }

  return QIcon();
}

QString Footage::duration()
{
  if (streams_.isEmpty()) {
    return QString();
  }

  if (streams_.first()->type() == Stream::kVideo) {
    VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(streams_.first());

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
  } else if (streams_.first()->type() == Stream::kAudio) {
    AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(streams_.first());

    // If we're showing in a timecode, we prefer showing audio in seconds instead
    Timecode::Display display = Core::instance()->GetTimecodeDisplay();
    if (display == Timecode::kTimecodeDropFrame
        || display == Timecode::kTimecodeNonDropFrame) {
      display = Timecode::kTimecodeSeconds;
    }

    return Timecode::timestamp_to_timecode(streams_.first()->duration(),
                                           streams_.first()->timebase(),
                                           display);
  }

  return QString();
}

QString Footage::rate()
{
  if (streams_.isEmpty()) {
    return QString();
  }

  if (streams_.first()->type() == Stream::kVideo) {
    // Return the timebase as a frame rate
    VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(streams_.first());

    return QCoreApplication::translate("Footage", "%1 FPS").arg(video_stream->frame_rate().toDouble());
  } else if (streams_.first()->type() == Stream::kAudio) {
    // Return the sample rate
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
  if (streams_.empty()) {
    return;
  }

  // Delete all streams
  streams_.clear();
}

bool Footage::HasStreamsOfType(const Stream::Type type)
{
  // Return true if any streams are video streams
  for (int i=0;i<streams_.size();i++) {
    if (streams_.at(i)->type() == type) {
      return true;
    }
  }

  return false;
}

void Footage::UpdateTooltip()
{
  switch (status_) {
  case kUnprobed:
    set_tooltip(QCoreApplication::translate("Footage", "Waiting for probe"));
    break;
  case kUnindexed:
    set_tooltip(QCoreApplication::translate("Footage", "Waiting for index"));
    break;
  case kReady:
  {
    QString tip = QCoreApplication::translate("Footage", "Filename: %1").arg(filename());

    if (!streams_.isEmpty()) {
      tip.append("\n");

      for (int i=0;i<streams_.size();i++) {

        StreamPtr s = streams_.at(i);

        switch (s->type()) {
        case Stream::kVideo:
        case Stream::kImage:
        {
          ImageStreamPtr vs = std::static_pointer_cast<ImageStream>(s);

          tip.append(
                QCoreApplication::translate("Footage",
                                            "\nVideo %1: %2x%3").arg(QString::number(i),
                                                                     QString::number(vs->width()),
                                                                     QString::number(vs->height()))
                );
          break;
        }
        case Stream::kAudio:
        {
          AudioStreamPtr as = std::static_pointer_cast<AudioStream>(s);

          tip.append(
                QCoreApplication::translate("Footage",
                                            "\nAudio %1: %2 channels %3 Hz").arg(QString::number(i),
                                                                                 QString::number(as->channels()),
                                                                                 QString::number(as->sample_rate()))
                );
          break;
        }
        default:
          break;
        }
      }
    }

    set_tooltip(tip);
  }
    break;
  case kInvalid:
    set_tooltip(QCoreApplication::translate("Footage", "An error occurred probing this footage"));
    break;
  }
}

OLIVE_NAMESPACE_EXIT
