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
  texture_input_ = new NodeInput("tex_in");
  texture_input_->set_data_type(NodeInput::kTexture);
  AddParameter(texture_input_);

  samples_input_ = new NodeInput("samples_in");
  samples_input_->set_data_type(NodeInput::kSamples);
  AddParameter(samples_input_);

  length_input_ = new NodeInput("length_in");
  length_input_->set_data_type(NodeInput::kRational);
  AddParameter(length_input_);
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

NodeInput *ViewerOutput::texture_input()
{
  return texture_input_;
}

NodeInput *ViewerOutput::samples_input()
{
  return samples_input_;
}

NodeInput *ViewerOutput::length_input()
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

const VideoParams &ViewerOutput::video_params()
{
  return video_params_;
}

const AudioParams &ViewerOutput::audio_params()
{
  return audio_params_;
}

void ViewerOutput::set_video_params(const VideoParams &video)
{
  video_params_ = video;

  emit SizeChanged(video_params_.width(), video_params_.height());
  emit TimebaseChanged(video_params_.time_base());
}

void ViewerOutput::set_audio_params(const AudioParams &audio)
{
  audio_params_ = audio;
}

rational ViewerOutput::Length()
{
  // FIXME: This is pretty messy, there's probably a better way...
  Node* connected_node = length_input_->get_connected_node();

  if (connected_node) {
    return connected_node->Value(NodeValueDatabase()).Get(NodeParam::kNumber).value<rational>();
  }

  return 0;
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
