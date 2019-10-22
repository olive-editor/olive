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

ViewerOutput::ViewerOutput() :
  viewer_width_(0),
  viewer_height_(0)
{
  texture_input_ = new NodeInput("tex_in");
  texture_input_->add_data_input(NodeInput::kTexture);
  AddParameter(texture_input_);

  samples_input_ = new NodeInput("samples_in");
  samples_input_->add_data_input(NodeInput::kSamples);
  AddParameter(samples_input_);

  length_input_ = new NodeInput("length_in");
  length_input_->add_data_input(NodeInput::kRational);
  AddParameter(length_input_);
}

QString ViewerOutput::Name()
{
  return tr("Viewer");
}

QString ViewerOutput::id()
{
  return "org.olivevideoeditor.Olive.vieweroutput";
}

QString ViewerOutput::Category()
{
  return tr("Output");
}

QString ViewerOutput::Description()
{
  return tr("Interface between a Viewer panel and the node system.");
}

const rational &ViewerOutput::Timebase()
{
  return timebase_;
}

void ViewerOutput::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  emit TimebaseChanged(timebase_);
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

RenderTexturePtr ViewerOutput::GetTexture(const rational &time)
{
  return texture_input_->get_value(time).value<RenderTexturePtr>();
}

QByteArray ViewerOutput::GetSamples(const rational &in, const rational &out)
{
  return samples_input_->get_value(in, out).toByteArray();
}

void ViewerOutput::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Node::InvalidateCache(start_range, end_range, from);

  emit TextureChangedBetween(start_range, end_range);

  SendInvalidateCache(start_range, end_range);
}

void ViewerOutput::SetViewerSize(const int &width, const int &height)
{
  viewer_width_ = width;
  viewer_height_ = height;

  emit SizeChanged(viewer_width_, viewer_height_);
}

const int &ViewerOutput::ViewerWidth()
{
  return viewer_width_;
}

const int &ViewerOutput::ViewerHeight()
{
  return viewer_height_;
}

rational ViewerOutput::Length()
{
  return length_input_->get_value(0).value<rational>();
}

QVariant ViewerOutput::Value(NodeOutput *output, const rational &in, const rational &out)
{
  Q_UNUSED(output)
  Q_UNUSED(in)
  Q_UNUSED(out)

  return 0;
}
