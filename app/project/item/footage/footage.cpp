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
#include "common/filefunctions.h"
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
  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("name")) {
      set_name(reader->readElementText());
    } else if (reader->name() == QStringLiteral("filename")) {
      set_filename(reader->readElementText());
    } else if (reader->name() == QStringLiteral("stream")) {
      add_stream(Stream::Load(reader, xml_node_data, cancelled));
    } else if (reader->name() == QStringLiteral("timestamp")) {
      set_timestamp(reader->readElementText().toLongLong());
    } else if (reader->name() == QStringLiteral("decoder")) {
      set_decoder(reader->readElementText());
    } else if (reader->name() == QStringLiteral("points")) {
      TimelinePoints::Load(reader);
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Footage::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("name"), name());
  writer->writeTextElement(QStringLiteral("filename"), filename());
  writer->writeTextElement(QStringLiteral("timestamp"), QString::number(timestamp_));
  writer->writeTextElement(QStringLiteral("decoder"), decoder_);

  writer->writeStartElement(QStringLiteral("points"));
    TimelinePoints::Save(writer);
  writer->writeEndElement(); // points

  foreach (StreamPtr stream, streams_) {
    writer->writeStartElement(QStringLiteral("stream"));
      stream->Save(writer);
    writer->writeEndElement(); // stream
  }
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

const qint64 &Footage::timestamp() const
{
  return timestamp_;
}

void Footage::set_timestamp(const qint64 &t)
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

bool Footage::CompareFootageToItsFilename(FootagePtr footage)
{
  // Heuristic to determine if file has changed
  QFileInfo info(footage->filename());

  if (info.exists()) {
    if (info.lastModified().toMSecsSinceEpoch() == footage->timestamp()) {
      // Footage has not been modified and is where we expect
      return true;
    } else {
      // Footage may have changed and we'll have to re-probe it. It also may not have, in which
      // case nothing needs to change.
      ItemPtr item = Decoder::Probe(footage->project(), footage->filename(), nullptr);

      if (item && item->type() == footage->type()) {
        // Item is the same type, that's a good sign. Let's look for any differences.
        // FIXME: Implement this
        return true;
      }
    }
  }

  // Footage file couldn't be found or resolved to something we didn't expect
  return false;
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
