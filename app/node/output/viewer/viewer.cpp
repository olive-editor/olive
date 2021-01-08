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

ViewerOutput::ViewerOutput() :
  video_frame_cache_(this),
  audio_playback_cache_(this),
  operation_stack_(0)
{
  texture_input_ = new NodeInput(this, QStringLiteral("tex_in"), NodeValue::kTexture);

  samples_input_ = new NodeInput(this, QStringLiteral("samples_in"), NodeValue::kSamples);

  // Create TrackList instances
  track_inputs_.resize(Timeline::kTrackTypeCount);
  track_lists_.resize(Timeline::kTrackTypeCount);

  for (int i=0;i<Timeline::kTrackTypeCount;i++) {
    // Create track input
    NodeInput* track_input = new NodeInput(this, QStringLiteral("track_in_%1").arg(i), NodeValue::kNone);
    IgnoreConnectionSignalsFrom(track_input);
    track_inputs_.replace(i, track_input);

    TrackList* list = new TrackList(this, static_cast<Timeline::TrackType>(i), track_input);
    track_lists_.replace(i, list);
    connect(list, &TrackList::TrackListChanged, this, &ViewerOutput::UpdateTrackCache);
    connect(list, &TrackList::LengthChanged, this, &ViewerOutput::VerifyLength);
    connect(list, &TrackList::BlockAdded, this, &ViewerOutput::TrackListAddedBlock);
    connect(list, &TrackList::BlockRemoved, this, &ViewerOutput::BlockRemoved);
    connect(list, &TrackList::TrackAdded, this, &ViewerOutput::TrackListAddedTrack);
    connect(list, &TrackList::TrackRemoved, this, &ViewerOutput::TrackRemoved);
    connect(list, &TrackList::TrackHeightChanged, this, &ViewerOutput::TrackHeightChangedSlot);
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

  foreach (TrackOutput* track, track_lists_.at(Timeline::kTrackTypeAudio)->GetTracks()) {
    track->waveform().Shift(from, to);
  }
}

void ViewerOutput::ShiftCache(const rational &from, const rational &to)
{
  ShiftVideoCache(from, to);
  ShiftAudioCache(from, to);
}

void ViewerOutput::InvalidateCache(const TimeRange& range, const InputConnection& from)
{
  if (operation_stack_ == 0) {
    if (from.input == texture_input_ || from.input == samples_input_) {
      TimeRange invalidated_range(qMax(rational(), range.in()),
                                  qMin(GetLength(), range.out()));

      if (invalidated_range.in() != invalidated_range.out()) {
        if (from.input == texture_input_) {
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

QVector<TrackOutput *> ViewerOutput::GetUnlockedTracks() const
{
  QVector<TrackOutput*> tracks = GetTracks();

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
    foreach (TrackOutput* track, list->GetTracks()) {
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
    video_length = track_lists_.at(Timeline::kTrackTypeVideo)->GetTotalLength();

    if (video_length.isNull() && texture_input_->IsConnected()) {
      NodeValueTable t = traverser.GenerateTable(texture_input_->GetConnectedNode(), 0, 0);
      video_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    video_frame_cache_.SetLength(video_length);
  }

  {
    audio_length = track_lists_.at(Timeline::kTrackTypeAudio)->GetTotalLength();

    if (audio_length.isNull() && samples_input_->IsConnected()) {
      NodeValueTable t = traverser.GenerateTable(samples_input_->GetConnectedNode(), 0, 0);
      audio_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    audio_playback_cache_.SetLength(audio_length);
  }

  {
    subtitle_length = track_lists_.at(Timeline::kTrackTypeSubtitle)->GetTotalLength();
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

  texture_input_->set_name(tr("Texture"));

  samples_input_->set_name(tr("Samples"));

  for (int i=0;i<track_inputs_.size();i++) {
    QString input_name;

    switch (static_cast<Timeline::TrackType>(i)) {
    case Timeline::kTrackTypeVideo:
      input_name = tr("Video Tracks");
      break;
    case Timeline::kTrackTypeAudio:
      input_name = tr("Audio Tracks");
      break;
    case Timeline::kTrackTypeSubtitle:
      input_name = tr("Subtitle Tracks");
      break;
    case Timeline::kTrackTypeNone:
    case Timeline::kTrackTypeCount:
      break;
    }

    if (!input_name.isEmpty()) {
      track_inputs_.at(i)->set_name(input_name);
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

void ViewerOutput::TrackListAddedBlock(Block *block, int index)
{
  Timeline::TrackType type = static_cast<TrackList*>(sender())->type();
  emit BlockAdded(block, TrackReference(type, index));
}

void ViewerOutput::TrackListAddedTrack(TrackOutput *track)
{
  Timeline::TrackType type = static_cast<TrackList*>(sender())->type();
  emit TrackAdded(track, type);
}

void ViewerOutput::TrackHeightChangedSlot(int index, int height)
{
  emit TrackHeightChanged(static_cast<TrackList*>(sender())->type(), index, height);
}

}
