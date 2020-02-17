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

#include "viewer.h"

ViewerOutput::ViewerOutput()
{
  texture_input_ = new NodeInput("tex_in", NodeInput::kTexture);
  AddInput(texture_input_);

  samples_input_ = new NodeInput("samples_in", NodeInput::kSamples);
  AddInput(samples_input_);

  length_input_ = new NodeInput("length_in", NodeInput::kRational);
  AddInput(length_input_);

  // Create TrackList instances
  track_inputs_.resize(Timeline::kTrackTypeCount);
  track_lists_.resize(Timeline::kTrackTypeCount);

  for (int i=0;i<Timeline::kTrackTypeCount;i++) {
    // Create track input
    NodeInputArray* track_input = new NodeInputArray(QStringLiteral("track_in_%1").arg(i), NodeParam::kAny);
    AddInput(track_input);
    track_inputs_.replace(i, track_input);

    TrackList* list = new TrackList(this, static_cast<Timeline::TrackType>(i), track_input);
    track_lists_.replace(i, list);
    connect(list, &TrackList::TrackListChanged, this, &ViewerOutput::UpdateTrackCache);
    connect(list, &TrackList::LengthChanged, this, &ViewerOutput::UpdateLength);
    connect(list, &TrackList::BlockAdded, this, &ViewerOutput::TrackListAddedBlock);
    connect(list, &TrackList::BlockRemoved, this, &ViewerOutput::BlockRemoved);
    connect(list, &TrackList::TrackAdded, this, &ViewerOutput::TrackListAddedTrack);
    connect(list, &TrackList::TrackRemoved, this, &ViewerOutput::TrackRemoved);
    connect(list, &TrackList::TrackHeightChanged, this, &ViewerOutput::TrackHeightChangedSlot);
  }

  // Create UUID for this node
  uuid_ = QUuid::createUuid();
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
  return "org.olivevideoeditor.Olive.vieweroutput";
}

QString ViewerOutput::Category() const
{
  return tr("Output");
}

QString ViewerOutput::Description() const
{
  return tr("Interface between a Viewer panel and the node system.");
}

NodeInput *ViewerOutput::texture_input() const
{
  return texture_input_;
}

NodeInput *ViewerOutput::samples_input() const
{
  return samples_input_;
}

NodeInput *ViewerOutput::length_input() const
{
  return length_input_;
}

void ViewerOutput::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Node::InvalidateCache(start_range, end_range, from);

  if (from == texture_input()) {
    emit VideoChangedBetween(start_range, end_range);
  } else if (from == samples_input()) {
    emit AudioChangedBetween(start_range, end_range);
  } else if (from == length_input()) {
    emit LengthChanged(Length());
  }

  SendInvalidateCache(start_range, end_range);
}

void ViewerOutput::InvalidateVisible(NodeInput* from)
{
  if (from == texture_input()) {
    emit VisibleInvalidated();
  }

  Node::InvalidateVisible(from);
}

const VideoParams &ViewerOutput::video_params() const
{
  return video_params_;
}

const AudioParams &ViewerOutput::audio_params() const
{
  return audio_params_;
}

void ViewerOutput::set_video_params(const VideoParams &video)
{
  video_params_ = video;

  emit SizeChanged(video_params_.width(), video_params_.height());
  emit TimebaseChanged(video_params_.time_base());
  emit VideoParamsChanged();
}

void ViewerOutput::set_audio_params(const AudioParams &audio)
{
  audio_params_ = audio;
}

rational ViewerOutput::Length()
{
  if (!length_input_->IsConnected()) {
    return timeline_length_;
  }

  Node* connected_node = length_input_->get_connected_node();

  if (connected_node) {
    // This is kind of messy?
    return connected_node->Value(NodeValueDatabase()).Get(NodeParam::kNumber, "length").value<rational>();
  }

  return 0;
}

const QUuid &ViewerOutput::uuid() const
{
  return uuid_;
}

void ViewerOutput::DependentEdgeChanged(NodeInput *from)
{
  if (from == texture_input_) {
    emit VideoGraphChanged();
  } else if (from == samples_input_) {
    emit AudioGraphChanged();
  }

  Node::DependentEdgeChanged(from);
}

void ViewerOutput::UpdateTrackCache()
{
  track_cache_.clear();

  foreach (TrackList* list, track_lists_) {
    QVector<TrackOutput*> track_list = list->Tracks();

    foreach (TrackOutput* track, track_list) {
      if (track) {
        track_cache_.append(list->Tracks());
      }
    }
  }
}

void ViewerOutput::UpdateLength(const rational &length)
{
  // If this length is equal, no-op
  if (length == timeline_length_) {
    return;
  }

  // If this length is greater, this must be the new total length
  if (length > timeline_length_) {
    timeline_length_ = length;
    emit LengthChanged(timeline_length_);
    return;
  }

  // Otherwise, the new length is shorter and we'll have to manually determine what the new max length is
  rational new_length = 0;

  foreach (TrackList* list, track_lists_) {
    new_length = qMax(new_length, list->TrackLength());
  }

  if (new_length != timeline_length_) {
    timeline_length_ = new_length;
    emit LengthChanged(timeline_length_);
  }
}

void ViewerOutput::Retranslate()
{
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

    if (!input_name.isEmpty())
      track_inputs_.at(i)->set_name(input_name);
  }
}

const QString &ViewerOutput::media_name() const
{
  return media_name_;
}

void ViewerOutput::set_media_name(const QString &name)
{
  media_name_ = name;

  emit MediaNameChanged(media_name_);
}

const QVector<TrackOutput *>& ViewerOutput::Tracks() const
{
  return track_cache_;
}

NodeInput *ViewerOutput::track_input(Timeline::TrackType type) const
{
  return track_inputs_.at(type);
}

TrackList *ViewerOutput::track_list(Timeline::TrackType type) const
{
  return track_lists_.at(type);
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
