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

#include "viewer.h"

#include "node/traverser.h"

namespace olive {

const QString ViewerOutput::kTextureInput = QStringLiteral("tex_in");
const QString ViewerOutput::kSamplesInput = QStringLiteral("samples_in");
const QString ViewerOutput::kTrackInputFormat = QStringLiteral("track_in_%1");

ViewerOutput::ViewerOutput() :
  video_frame_cache_(this),
  audio_playback_cache_(this),
  operation_stack_(0)
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));

  // Create TrackList instances
  track_lists_.resize(Track::kCount);

  for (int i=0;i<Track::kCount;i++) {
    // Create track input
    QString track_input_id = kTrackInputFormat.arg(i);

    AddInput(track_input_id, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable | kInputFlagArray));

    IgnoreInvalidationsFrom(track_input_id);

    TrackList* list = new TrackList(this, static_cast<Track::Type>(i), track_input_id);
    track_lists_.replace(i, list);
    connect(list, &TrackList::TrackListChanged, this, &ViewerOutput::UpdateTrackCache);
    connect(list, &TrackList::LengthChanged, this, &ViewerOutput::VerifyLength);
    connect(list, &TrackList::TrackAdded, this, &ViewerOutput::TrackAdded);
    connect(list, &TrackList::TrackRemoved, this, &ViewerOutput::TrackRemoved);
  }

  // Create UUID for this node
  uuid_ = QUuid::createUuid();
}

ViewerOutput::~ViewerOutput()
{
  DisconnectAll();
}

Node *ViewerOutput::copy() const
{
  return new ViewerOutput();
}

QString ViewerOutput::Name() const
{
  return tr("Viewer");
}

QString ViewerOutput::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.vieweroutput");
}

QVector<Node::CategoryID> ViewerOutput::Category() const
{
  return {kCategoryOutput};
}

QString ViewerOutput::Description() const
{
  return tr("Interface between a Viewer panel and the node system.");
}

void ViewerOutput::ShiftVideoCache(const rational &from, const rational &to)
{
  video_frame_cache_.Shift(from, to);
}

void ViewerOutput::ShiftAudioCache(const rational &from, const rational &to)
{
  audio_playback_cache_.Shift(from, to);

  foreach (Track* track, track_lists_.at(Track::kAudio)->GetTracks()) {
    track->waveform().Shift(from, to);
  }
}

void ViewerOutput::ShiftCache(const rational &from, const rational &to)
{
  ShiftVideoCache(from, to);
  ShiftAudioCache(from, to);
}

void ViewerOutput::InvalidateCache(const TimeRange& range, const QString& from, int element)
{
  Q_UNUSED(element)

  if (operation_stack_ == 0) {
    if (from == kTextureInput || from == kSamplesInput) {
      TimeRange invalidated_range(qMax(rational(), range.in()),
                                  qMin(GetLength(), range.out()));

      if (invalidated_range.in() != invalidated_range.out()) {
        if (from == kTextureInput) {
          video_frame_cache_.Invalidate(invalidated_range);
        } else {
          audio_playback_cache_.Invalidate(invalidated_range);
        }
      }
    }

    VerifyLength();
  }

  Node::InvalidateCache(range, from);
}

void ViewerOutput::set_video_params(const VideoParams &video)
{
  bool size_changed = video_params_.width() != video.width() || video_params_.height() != video.height();
  bool timebase_changed = video_params_.time_base() != video.time_base();
  bool pixel_aspect_changed = video_params_.pixel_aspect_ratio() != video.pixel_aspect_ratio();
  bool interlacing_changed = video_params_.interlacing() != video.interlacing();

  video_params_ = video;

  if (size_changed) {
    emit SizeChanged(video_params_.width(), video_params_.height());
  }

  if (pixel_aspect_changed) {
    emit PixelAspectChanged(video_params_.pixel_aspect_ratio());
  }

  if (interlacing_changed) {
    emit InterlacingChanged(video_params_.interlacing());
  }

  if (timebase_changed) {
    video_frame_cache_.SetTimebase(video_params_.time_base());
    emit TimebaseChanged(video_params_.time_base());
  }

  emit VideoParamsChanged();

  video_frame_cache_.InvalidateAll();
}

void ViewerOutput::set_audio_params(const AudioParams &audio)
{
  audio_params_ = audio;

  emit AudioParamsChanged();

  // This will automatically InvalidateAll
  audio_playback_cache_.SetParameters(audio_params());
}

rational ViewerOutput::GetLength()
{
  return last_length_;
}

QVector<Track *> ViewerOutput::GetUnlockedTracks() const
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

void ViewerOutput::UpdateTrackCache()
{
  track_cache_.clear();

  foreach (TrackList* list, track_lists_) {
    foreach (Track* track, list->GetTracks()) {
      track_cache_.append(track);
    }
  }
}

void ViewerOutput::VerifyLength()
{
  if (operation_stack_ != 0) {
    return;
  }

  NodeTraverser traverser;

  rational video_length, audio_length, subtitle_length;

  {
    video_length = track_lists_.at(Track::kVideo)->GetTotalLength();

    if (video_length.isNull() && IsInputConnected(kTextureInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kTextureInput), TimeRange(0, 0));
      video_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    video_frame_cache_.SetLength(video_length);
  }

  {
    audio_length = track_lists_.at(Track::kAudio)->GetTotalLength();

    if (audio_length.isNull() && IsInputConnected(kSamplesInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kSamplesInput), TimeRange(0, 0));
      audio_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    audio_playback_cache_.SetLength(audio_length);
  }

  {
    subtitle_length = track_lists_.at(Track::kSubtitle)->GetTotalLength();
  }

  rational real_length = qMax(subtitle_length, qMax(video_length, audio_length));

  if (real_length != last_length_) {
    last_length_ = real_length;
    emit LengthChanged(last_length_);
  }
}

void ViewerOutput::Retranslate()
{
  Node::Retranslate();

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

void ViewerOutput::BeginOperation()
{
  operation_stack_++;

  Node::BeginOperation();
}

void ViewerOutput::EndOperation()
{
  operation_stack_--;

  Node::EndOperation();
}

void ViewerOutput::InputConnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  } else {
    foreach (TrackList* list, track_lists_) {
      if (list->track_input() == input) {
        list->TrackConnected(output.node(), element);
        break;
      }
    }
  }
}

void ViewerOutput::InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  } else {
    foreach (TrackList* list, track_lists_) {
      if (list->track_input() == input) {
        list->TrackDisconnected(output.node(), element);
        break;
      }
    }
  }
}

}
