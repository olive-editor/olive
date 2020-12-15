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

#include "sequence.h"

#include <QCoreApplication>
#include <QThread>

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

namespace olive {

Sequence::Sequence()
{
  viewer_output_ = new ViewerOutput();
  viewer_output_->SetCanBeDeleted(false);
  AddNode(viewer_output_);
}

void Sequence::Load(QXmlStreamReader *reader, XMLNodeData& xml_node_data, uint version, const QAtomicInt *cancelled)
{
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
  }

  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("video")) {
      int video_width = 0, video_height = 0, preview_div = 1;
      rational video_timebase, video_pixel_aspect;
      VideoParams::Interlacing video_interlacing = VideoParams::kInterlaceNone;
      VideoParams::Format preview_format = VideoParams::kFormatInvalid;

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
          preview_format = static_cast<VideoParams::Format>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("pixelaspect")) {
          video_pixel_aspect = rational::fromString(reader->readElementText());
        } else if (reader->name() == QStringLiteral("interlacing")) {
          video_interlacing = static_cast<VideoParams::Interlacing>(reader->readElementText().toInt());
        } else {
          reader->skipCurrentElement();
        }
      }

      set_video_params(VideoParams(video_width, video_height, video_timebase, preview_format,
                                   VideoParams::kInternalChannelCount, video_pixel_aspect,
                                   video_interlacing, preview_div));
    } else if (reader->name() == QStringLiteral("audio")) {
      int rate = 0;
      uint64_t layout = 0;
      AudioParams::Format format = AudioParams::kFormatInvalid;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("rate")) {
          rate = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("layout")) {
          layout = reader->readElementText().toULongLong();
        } else if (reader->name() == QStringLiteral("format")) {
          format = static_cast<AudioParams::Format>(reader->readElementText().toInt());
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
        node = nullptr;

        {
          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("id")) {
              QString id = attr.value().toString();
              if (version <= 201003) {
                // After version 201003, the video and audio nodes were merged into one media node
                if (id == QStringLiteral("org.olivevideoeditor.Olive.audioinput")
                    || id == QStringLiteral("org.olivevideoeditor.Olive.videoinput")) {
                  id = QStringLiteral("org.olivevideoeditor.Olive.mediainput");
                }
              }

              if (version <= 201118) {
                // After version 201118, the orthographic matrix node ID was changed from
                // "org.olivevideoeditor.Olive.transform" to "org.olivevideoeditor.Olive.ortho"
                if (id == QStringLiteral("org.olivevideoeditor.Olive.transform")) {
                  id = QStringLiteral("org.olivevideoeditor.Olive.ortho");
                }
              }

              node = NodeFactory::CreateFromID(id);
              break;
            }
          }
        }
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
}

void Sequence::Save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("name"), name());

  writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(this)));

  writer->writeStartElement(QStringLiteral("video"));

  writer->writeTextElement(QStringLiteral("width"), QString::number(video_params().width()));
  writer->writeTextElement(QStringLiteral("height"), QString::number(video_params().height()));
  writer->writeTextElement(QStringLiteral("timebase"), video_params().time_base().toString());
  writer->writeTextElement(QStringLiteral("pixelaspect"), video_params().pixel_aspect_ratio().toString());
  writer->writeTextElement(QStringLiteral("interlacing"), QString::number(video_params().interlacing()));
  writer->writeTextElement(QStringLiteral("divider"), QString::number(video_params().divider()));
  writer->writeTextElement(QStringLiteral("format"), QString::number(video_params().format()));

  writer->writeEndElement(); // video

  writer->writeStartElement(QStringLiteral("audio"));

  writer->writeTextElement(QStringLiteral("rate"), QString::number(audio_params().sample_rate()));
  writer->writeTextElement(QStringLiteral("layout"), QString::number(audio_params().channel_layout()));
  writer->writeTextElement(QStringLiteral("format"), QString::number(audio_params().format()));

  writer->writeEndElement(); // audio

  // Write TimelinePoints
  writer->writeStartElement(QStringLiteral("points"));
    TimelinePoints::Save(writer);
  writer->writeEndElement(); // points

  foreach (Node* node, nodes()) {
    if (node != viewer_output_) {
      writer->writeStartElement(QStringLiteral("node"));
      writer->writeAttribute(QStringLiteral("id"), node->id());
      node->Save(writer);
      writer->writeEndElement(); // node;
    }
  }

  writer->writeStartElement(QStringLiteral("viewer"));
  writer->writeAttribute(QStringLiteral("id"), viewer_output_->id());
  viewer_output_->Save(writer);
  writer->writeEndElement(); // viewer;
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
                               static_cast<VideoParams::Format>(Config::Current()["OfflinePixelFormat"].toInt()),
                               VideoParams::kInternalChannelCount,
                               Config::Current()["DefaultSequencePixelAspect"].value<rational>(),
                               Config::Current()["DefaultSequenceInterlacing"].value<VideoParams::Interlacing>(),
                               VideoParams::generate_auto_divider(width, height)));
  set_audio_params(AudioParams(Config::Current()["DefaultSequenceAudioFrequency"].toInt(),
                   Config::Current()["DefaultSequenceAudioLayout"].toULongLong(),
                   AudioParams::kInternalFormat));
}

void Sequence::set_parameters_from_footage(const QList<Footage *> footage)
{
  bool found_video_params = false;
  bool found_audio_params = false;

  foreach (Footage* f, footage) {
    foreach (StreamPtr s, f->streams()) {
      if (!s->enabled()) {
        continue;
      }

      switch (s->type()) {
      case Stream::kVideo:
      {
        VideoStream* vs = static_cast<VideoStream*>(s.get());

        // If this is a video stream, use these parameters
        if (!found_video_params) {
          rational using_timebase;

          if (vs->video_type() == VideoStream::kVideoTypeStill) {
            // If this is a still image, we'll use it's resolution but won't set
            // `found_video_params` in case something with a frame rate comes along which we'll
            // prioritize
            using_timebase = video_params().time_base();
          } else {
            using_timebase = vs->frame_rate().flipped();
            found_video_params = true;
          }

          set_video_params(VideoParams(vs->width(),
                                       vs->height(),
                                       using_timebase,
                                       static_cast<VideoParams::Format>(Config::Current()["OfflinePixelFormat"].toInt()),
                                       VideoParams::kInternalChannelCount,
                                       vs->pixel_aspect_ratio(),
                                       vs->interlacing(),
                                       VideoParams::generate_auto_divider(vs->width(), vs->height())));
        }
        break;
      }
      case Stream::kAudio:
        if (!found_audio_params) {
          AudioStream* as = static_cast<AudioStream*>(s.get());
          set_audio_params(AudioParams(as->sample_rate(), as->channel_layout(), AudioParams::kInternalFormat));
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

}
