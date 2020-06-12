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

#include "sequence.h"

#include <QCoreApplication>

#include "config/config.h"
#include "common/channellayout.h"
#include "common/timecodefunctions.h"
#include "common/xmlutils.h"
#include "node/factory.h"
#include "panel/panelmanager.h"
#include "panel/node/node.h"
#include "panel/curve/curve.h"
#include "panel/param/param.h"
#include "panel/timeline/timeline.h"
#include "panel/sequenceviewer/sequenceviewer.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

Sequence::Sequence()
{
  viewer_output_ = new ViewerOutput();
  viewer_output_->SetCanBeDeleted(false);
  AddNode(viewer_output_);
}

void Sequence::Load(QXmlStreamReader *reader, XMLNodeData& xml_node_data, const QAtomicInt *cancelled)
{
  XMLAttributeLoop(reader, attr) {
    if (cancelled && *cancelled) {
      return;
    }

    if (attr.name() == QStringLiteral("name")) {
      set_name(attr.value().toString());
    } else if (attr.name() == QStringLiteral("ptr")) {
      xml_node_data.item_ptrs.insert(attr.value().toULongLong(), this);
    }
  }

  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("video")) {
      int video_width = 0, video_height = 0, preview_div = 1;
      rational video_timebase;
      PixelFormat::Format preview_format = PixelFormat::PIX_FMT_INVALID;

      while (XMLReadNextStartElement(reader)) {
        if (cancelled && *cancelled) {
          return;
        }

        if (reader->name() == QStringLiteral("width")) {
          video_width = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("height")) {
          video_height = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("timebase")) {
          video_timebase = rational::fromString(reader->readElementText());
        } else if (reader->name() == QStringLiteral("divider")) {
          preview_div = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("format")) {
          preview_format = static_cast<PixelFormat::Format>(reader->readElementText().toInt());
        } else {
          reader->skipCurrentElement();
        }
      }

      set_video_params(VideoParams(video_width, video_height, video_timebase, preview_format, preview_div));
    } else if (reader->name() == QStringLiteral("audio")) {
      int rate = 0;
      uint64_t layout = 0;
      SampleFormat::Format format = SampleFormat::SAMPLE_FMT_INVALID;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("rate")) {
          rate = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("layout")) {
          layout = reader->readElementText().toULongLong();
        } else if (reader->name() == QStringLiteral("format")) {
          format = static_cast<SampleFormat::Format>(reader->readElementText().toInt());
        } else {
          reader->skipCurrentElement();
        }
      }

      set_audio_params(AudioParams(rate, layout, format));
    } else if (reader->name() == QStringLiteral("points")) {

      TimelinePoints::Load(reader);

    } else if (reader->name() == QStringLiteral("node") || reader->name() == QStringLiteral("viewer")) {
      Node* node;

      if (reader->name() == QStringLiteral("node")) {
        node = XMLLoadNode(reader);
      } else {
        node = viewer_output_;
      }

      if (node) {
        node->Load(reader, xml_node_data, cancelled);

        AddNode(node);
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  // Make connections
  XMLConnectNodes(xml_node_data);

  // Link blocks
  XMLLinkBlocks(xml_node_data);

  // Ensure this and all children are in the main thread
  // (FIXME: Weird place for this? This should probably be in ProjectLoadManager somehow)
  if (thread() != qApp->thread()) {
    moveToThread(qApp->thread());
  }
}

void Sequence::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement(QStringLiteral("sequence"));

  writer->writeAttribute(QStringLiteral("name"), name());

  writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(viewer_output_)));

  writer->writeStartElement(QStringLiteral("video"));

  writer->writeTextElement(QStringLiteral("width"), QString::number(video_params().width()));
  writer->writeTextElement(QStringLiteral("height"), QString::number(video_params().height()));
  writer->writeTextElement(QStringLiteral("timebase"), video_params().time_base().toString());
  writer->writeTextElement(QStringLiteral("divider"), QString::number(video_params().divider()));
  writer->writeTextElement(QStringLiteral("format"), QString::number(video_params().format()));

  writer->writeEndElement(); // video

  writer->writeStartElement(QStringLiteral("audio"));

  writer->writeTextElement(QStringLiteral("rate"), QString::number(audio_params().sample_rate()));
  writer->writeTextElement(QStringLiteral("layout"), QString::number(audio_params().channel_layout()));
  writer->writeTextElement(QStringLiteral("format"), QString::number(audio_params().format()));

  writer->writeEndElement(); // audio

  // Write TimelinePoints
  TimelinePoints::Save(writer);

  foreach (Node* node, nodes()) {
    if (node != viewer_output_) {
      node->Save(writer);
    }
  }

  viewer_output_->Save(writer, QStringLiteral("viewer"));

  writer->writeEndElement(); // sequence
}

void Sequence::add_default_nodes()
{
  // Create tracks and connect them to the viewer
  Node* video_track_output = viewer_output_->track_list(Timeline::kTrackTypeVideo)->AddTrack();
  Node* audio_track_output = viewer_output_->track_list(Timeline::kTrackTypeAudio)->AddTrack();
  NodeParam::ConnectEdge(video_track_output->output(), viewer_output_->texture_input());
  NodeParam::ConnectEdge(audio_track_output->output(), viewer_output_->samples_input());
}

Item::Type Sequence::type() const
{
  return kSequence;
}

QIcon Sequence::icon()
{
  return icon::Sequence;
}

QString Sequence::duration()
{
  rational timeline_length = viewer_output_->GetLength();

  int64_t timestamp = Timecode::time_to_timestamp(timeline_length, video_params().time_base());

  return Timecode::timestamp_to_timecode(timestamp, video_params().time_base(), Core::instance()->GetTimecodeDisplay());
}

QString Sequence::rate()
{
  return QCoreApplication::translate("Sequence", "%1 FPS").arg(video_params().time_base().flipped().toDouble());
}

const VideoParams &Sequence::video_params() const
{
  return viewer_output_->video_params();
}

void Sequence::set_video_params(const VideoParams &vparam)
{
  viewer_output_->set_video_params(vparam);
}

const AudioParams &Sequence::audio_params() const
{
  return viewer_output_->audio_params();
}

void Sequence::set_audio_params(const AudioParams &params)
{
  viewer_output_->set_audio_params(params);
}

void Sequence::set_default_parameters()
{
  int width = Config::Current()["DefaultSequenceWidth"].toInt();
  int height = Config::Current()["DefaultSequenceHeight"].toInt();

  set_video_params(VideoParams(width,
                               height,
                               Config::Current()["DefaultSequenceFrameRate"].value<rational>(),
                               static_cast<PixelFormat::Format>(Config::Current()["DefaultSequencePreviewFormat"].toInt()),
                               VideoParams::generate_auto_divider(width, height)));
  set_audio_params(AudioParams(Config::Current()["DefaultSequenceAudioFrequency"].toInt(),
                   Config::Current()["DefaultSequenceAudioLayout"].toULongLong(),
                   SampleFormat::kInternalFormat));
}

void Sequence::set_parameters_from_footage(const QList<Footage *> footage)
{
  bool found_video_params = false;
  bool found_audio_params = false;

  foreach (Footage* f, footage) {
    foreach (StreamPtr s, f->streams()) {
      switch (s->type()) {
      case Stream::kVideo:
      {
        VideoStream* vs = static_cast<VideoStream*>(s.get());

        // If this is a video stream, use these parameters
        if (!found_video_params && !vs->frame_rate().isNull()) {
          set_video_params(VideoParams(vs->width(),
                                       vs->height(),
                                       vs->frame_rate().flipped(),
                                       static_cast<PixelFormat::Format>(Config::Current()["DefaultSequencePreviewFormat"].toInt()),
                                       VideoParams::generate_auto_divider(vs->width(), vs->height())));
          found_video_params = true;
        }
        break;
      }
      case Stream::kImage:
        if (!found_video_params) {
          // If this is an image stream, we'll use it's resolution but won't set `found_video_params` in case
          // something with a frame rate comes along which we'll prioritize
          ImageStream* is = static_cast<ImageStream*>(s.get());

          set_video_params(VideoParams(is->width(),
                                       is->height(),
                                       video_params().time_base(),
                                       static_cast<PixelFormat::Format>(Config::Current()["DefaultSequencePreviewFormat"].toInt()),
                                       VideoParams::generate_auto_divider(is->width(), is->height())));
        }
        break;
      case Stream::kAudio:
        if (!found_audio_params) {
          AudioStream* as = static_cast<AudioStream*>(s.get());
          set_audio_params(AudioParams(as->sample_rate(), as->channel_layout(), SampleFormat::kInternalFormat));
          found_audio_params = true;
        }
        break;
      case Stream::kUnknown:
      case Stream::kData:
      case Stream::kSubtitle:
      case Stream::kAttachment:
        // Ignore these types
        break;
      }

      if (found_video_params && found_audio_params) {
        return;
      }
    }
  }
}

ViewerOutput *Sequence::viewer_output() const
{
  return viewer_output_;
}

void Sequence::NameChangedEvent(const QString &name)
{
  viewer_output_->set_media_name(name);
}

OLIVE_NAMESPACE_EXIT
