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
#include "widget/videoparamedit/videoparamedit.h"

namespace olive {

const QString Sequence::kVideoParamsInput = QStringLiteral("video_param_in");
const QString Sequence::kAudioParamsInput = QStringLiteral("audio_param_in");
const QString Sequence::kTextureInput = QStringLiteral("tex_in");
const QString Sequence::kSamplesInput = QStringLiteral("samples_in");
const QString Sequence::kTrackInputFormat = QStringLiteral("track_in_%1");

const uint64_t Sequence::kVideoParamEditMask = VideoParamEdit::kWidthHeight | VideoParamEdit::kInterlacing | VideoParamEdit::kFrameRate | VideoParamEdit::kPixelAspect;

#define super Node

Sequence::Sequence(bool viewer_only_mode) :
  video_frame_cache_(this),
  audio_playback_cache_(this),
  operation_stack_(0)
{
  AddInput(kVideoParamsInput, NodeValue::kVideoParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  SetInputProperty(kVideoParamsInput, QStringLiteral("mask"), QVariant::fromValue(kVideoParamEditMask));

  AddInput(kAudioParamsInput, NodeValue::kAudioParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
  AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));

  if (!viewer_only_mode) {
    // Create TrackList instances
    track_lists_.resize(Track::kCount);

    for (int i=0;i<Track::kCount;i++) {
      // Create track input
      QString track_input_id = kTrackInputFormat.arg(i);

      AddInput(track_input_id, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable | kInputFlagArray));

      IgnoreInvalidationsFrom(track_input_id);

      TrackList* list = new TrackList(this, static_cast<Track::Type>(i), track_input_id);
      track_lists_.replace(i, list);
      connect(list, &TrackList::TrackListChanged, this, &Sequence::UpdateTrackCache);
      connect(list, &TrackList::LengthChanged, this, &Sequence::VerifyLength);
      connect(list, &TrackList::TrackAdded, this, &Sequence::TrackAdded);
      connect(list, &TrackList::TrackRemoved, this, &Sequence::TrackRemoved);
    }
  }

  // Create UUID for this node
  uuid_ = QUuid::createUuid();
}

Sequence::~Sequence()
{
  DisconnectAll();
}

void Sequence::add_default_nodes(MultiUndoCommand* command)
{
  // Create tracks and connect them to the viewer
  command->add_child(new TimelineAddTrackCommand(track_list(Track::kVideo)));
  command->add_child(new TimelineAddTrackCommand(track_list(Track::kAudio)));
}

QIcon Sequence::icon() const
{
  return icon::Sequence;
}

QString Sequence::duration() const
{
  rational timeline_length = GetLength();

  int64_t timestamp = Timecode::time_to_timestamp(timeline_length, video_params().time_base());

  return Timecode::timestamp_to_timecode(timestamp, video_params().time_base(), Core::instance()->GetTimecodeDisplay());
}

QString Sequence::rate() const
{
  return tr("%1 FPS").arg(video_params().time_base().flipped().toDouble());
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

void Sequence::set_parameters_from_footage(const QVector<Footage *> footage)
{

  foreach (Footage* f, footage) {
    QVector<VideoParams> video_streams = f->GetEnabledVideoStreams();
    QVector<AudioParams> audio_streams = f->GetEnabledAudioStreams();

    foreach (const VideoParams& s, video_streams) {
      bool found_video_params = false;
      rational using_timebase;

      if (s.video_type() == VideoParams::kVideoTypeStill) {
        // If this is a still image, we'll use it's resolution but won't set
        // `found_video_params` in case something with a frame rate comes along which we'll
        // prioritize
        using_timebase = video_params().time_base();
      } else {
        using_timebase = s.frame_rate().flipped();
        found_video_params = true;
      }

      set_video_params(VideoParams(s.width(),
                                   s.height(),
                                   using_timebase,
                                   static_cast<VideoParams::Format>(Config::Current()[QStringLiteral("OfflinePixelFormat")].toInt()),
                                   VideoParams::kInternalChannelCount,
                                   s.pixel_aspect_ratio(),
                                   s.interlacing(),
                                   VideoParams::generate_auto_divider(s.width(), s.height())));

      if (found_video_params) {
        break;
      }
    }

    if (!audio_streams.isEmpty()) {
      const AudioParams& s = audio_streams.first();
      set_audio_params(AudioParams(s.sample_rate(), s.channel_layout(), AudioParams::kInternalFormat));
    }
  }
}

QVector<Track *> Sequence::GetUnlockedTracks() const
{
  QVector<Track*> tracks = GetTracks();

  for (int i=0;i<tracks.size();i++) {
    if (tracks.at(i)->IsLocked()) {
      tracks.removeAt(i);
      i--;
    }
  }

  return tracks;
}

void Sequence::Retranslate()
{
  super::Retranslate();

  SetInputName(kVideoParamsInput, tr("Video Parameters"));
  SetInputName(kAudioParamsInput, tr("Audio Parameters"));

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kSamplesInput, tr("Samples"));

  for (int i=0;i<Track::kCount;i++) {
    QString input_name;

    switch (static_cast<Track::Type>(i)) {
    case Track::kVideo:
      input_name = tr("Video Tracks");
      break;
    case Track::kAudio:
      input_name = tr("Audio Tracks");
      break;
    case Track::kSubtitle:
      input_name = tr("Subtitle Tracks");
      break;
    case Track::kNone:
    case Track::kCount:
      break;
    }

    if (!input_name.isEmpty()) {
      SetInputName(kTrackInputFormat.arg(i), input_name);
    }
  }
}

rational Sequence::GetCustomLength(Track::Type type) const
{
  if (!track_lists_.isEmpty()) {
    switch (type) {
    case Track::kVideo:
      return track_lists_.at(Track::kVideo)->GetTotalLength();
    case Track::kAudio:
      return track_lists_.at(Track::kAudio)->GetTotalLength();
    case Track::kSubtitle:
      return track_lists_.at(Track::kSubtitle)->GetTotalLength();
    case Track::kNone:
    case Track::kCount:
      break;
    }
  }

  return rational();
}

void Sequence::InputConnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  } else {
    foreach (TrackList* list, track_lists_) {
      if (list->track_input() == input) {
        // Return because we found our input
        list->TrackConnected(output.node(), element);
        return;
      }
    }
  }

  super::InputConnectedEvent(input, element, output);
}

void Sequence::InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  } else {
    foreach (TrackList* list, track_lists_) {
      if (list->track_input() == input) {
        // Return because we found our input
        list->TrackDisconnected(output.node(), element);
        return;
      }
    }
  }

  super::InputDisconnectedEvent(input, element, output);
}

void Sequence::ShiftAudioEvent(const rational &from, const rational &to)
{
  foreach (Track* track, track_lists_.at(Track::kAudio)->GetTracks()) {
    track->waveform().Shift(from, to);
  }
}

void Sequence::UpdateTrackCache()
{
  track_cache_.clear();

  foreach (TrackList* list, track_lists_) {
    foreach (Track* track, list->GetTracks()) {
      track_cache_.append(track);
    }
  }
}

void Sequence::ShiftVideoCache(const rational &from, const rational &to)
{
  video_frame_cache_.Shift(from, to);

  ShiftVideoEvent(from, to);
}

void Sequence::ShiftAudioCache(const rational &from, const rational &to)
{
  audio_playback_cache_.Shift(from, to);

  ShiftAudioEvent(from, to);
}

void Sequence::ShiftCache(const rational &from, const rational &to)
{
  ShiftVideoCache(from, to);
  ShiftAudioCache(from, to);
}

void Sequence::InvalidateCache(const TimeRange& range, const QString& from, int element, qint64 job_time)
{
  Q_UNUSED(element)

  if (operation_stack_ == 0) {
    if (from == kTextureInput || from == kSamplesInput) {
      TimeRange invalidated_range(qMax(rational(), range.in()),
                                  qMin(GetLength(), range.out()));

      if (invalidated_range.in() != invalidated_range.out()) {
        if (from == kTextureInput) {
          video_frame_cache_.Invalidate(invalidated_range, job_time);
        } else {
          audio_playback_cache_.Invalidate(invalidated_range, job_time);
        }
      }
    }

    VerifyLength();
  }

  super::InvalidateCache(range, from, element, job_time);
}

const rational& Sequence::GetLength() const
{
  return last_length_;
}

void Sequence::VerifyLength()
{
  if (operation_stack_ != 0) {
    return;
  }

  NodeTraverser traverser;

  rational video_length, audio_length, subtitle_length;

  {
    video_length = GetCustomLength(Track::kVideo);

    if (video_length.isNull() && IsInputConnected(kTextureInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kTextureInput), TimeRange(0, 0));
      video_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    video_frame_cache_.SetLength(video_length);
  }

  {
    audio_length = GetCustomLength(Track::kAudio);

    if (audio_length.isNull() && IsInputConnected(kSamplesInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kSamplesInput), TimeRange(0, 0));
      audio_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    audio_playback_cache_.SetLength(audio_length);
  }

  {
    subtitle_length = GetCustomLength(Track::kSubtitle);
  }

  rational real_length = qMax(subtitle_length, qMax(video_length, audio_length));

  if (real_length != last_length_) {
    last_length_ = real_length;
    emit LengthChanged(last_length_);
  }
}

void Sequence::BeginOperation()
{
  operation_stack_++;

  super::BeginOperation();
}

void Sequence::EndOperation()
{
  operation_stack_--;

  super::EndOperation();
}

void Sequence::LoadInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt *cancelled)
{
  Q_UNUSED(xml_node_data)
  Q_UNUSED(version)
  Q_UNUSED(cancelled)

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("points")) {
      timeline_points_.Load(reader);
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Sequence::SaveInternal(QXmlStreamWriter *writer) const
{
  // Write TimelinePoints
  writer->writeStartElement(QStringLiteral("points"));
  timeline_points_.Save(writer);
  writer->writeEndElement(); // points
}

void Sequence::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kVideoParamsInput) {

    VideoParams new_video_params = video_params();

    bool size_changed = cached_video_params_.width() != new_video_params.width() || cached_video_params_.height() != new_video_params.height();
    bool timebase_changed = cached_video_params_.time_base() != new_video_params.time_base();
    bool pixel_aspect_changed = cached_video_params_.pixel_aspect_ratio() != new_video_params.pixel_aspect_ratio();
    bool interlacing_changed = cached_video_params_.interlacing() != new_video_params.interlacing();

    if (size_changed) {
      emit SizeChanged(new_video_params.width(), new_video_params.height());
    }

    if (pixel_aspect_changed) {
      emit PixelAspectChanged(new_video_params.pixel_aspect_ratio());
    }

    if (interlacing_changed) {
      emit InterlacingChanged(new_video_params.interlacing());
    }

    if (timebase_changed) {
      video_frame_cache_.SetTimebase(new_video_params.time_base());
      emit TimebaseChanged(new_video_params.time_base());
    }

    emit VideoParamsChanged();

    cached_video_params_ = video_params();

  } else if (input == kAudioParamsInput) {

    emit AudioParamsChanged();

    // This will automatically InvalidateAll
    audio_playback_cache_.SetParameters(audio_params());

  }
}

void Sequence::ShiftVideoEvent(const rational &from, const rational &to)
{
  Q_UNUSED(from)
  Q_UNUSED(to)
}

}
