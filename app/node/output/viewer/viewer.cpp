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
  attached_viewer_(nullptr)
{
  texture_input_ = new NodeInput("tex_out");
  texture_input_->add_data_input(NodeInput::kTexture);
  AddParameter(texture_input_);
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

void ViewerOutput::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  if (attached_viewer_ != nullptr) {
    attached_viewer_->SetTimebase(timebase_);
  }
}

NodeInput *ViewerOutput::texture_input()
{
  return texture_input_;
}

void ViewerOutput::AttachViewer(ViewerPanel *viewer)
{
  // Disconnect old viewer if there's one attached
  if (attached_viewer_ != nullptr) {
    disconnect(attached_viewer_, SIGNAL(TimeChanged(const rational&)), this, SLOT(ViewerTimeChanged(const rational&)));

    // Clear any existing texture
    attached_viewer_->SetTexture(0);
  }

  // FIXME: Currently this attaches to ViewerPanels, but should it attached to Viewers instead?
  attached_viewer_ = viewer;

  if (attached_viewer_ != nullptr) {
    connect(attached_viewer_, SIGNAL(TimeChanged(const rational&)), this, SLOT(ViewerTimeChanged(const rational&)));
    SetTimebase(timebase_);

    // Update the texture
    ViewerTimeChanged(attached_viewer_->GetTime());
  }
}

void ViewerOutput::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Node::InvalidateCache(start_range, end_range, from);

  if (attached_viewer_ != nullptr
      && (start_range == attached_viewer_->GetTime() || end_range == attached_viewer_->GetTime())) {
    // Update any attached viewer
    ForceUpdateViewer();
  }

  SendInvalidateCache(start_range, end_range);
}

void ViewerOutput::ForceUpdateViewer()
{
  ViewerTimeChanged(attached_viewer_->GetTime());
}

QVariant ViewerOutput::Value(NodeOutput *output, const rational &time)
{
  Q_UNUSED(output)
  Q_UNUSED(time)

  return 0;
}

void ViewerOutput::ViewerTimeChanged(const rational &t)
{
  // Get the texture from whatever Node is currently connected (usually a Renderer of some kind)
  RenderTexturePtr current_texture = texture_input_->get_value(t).value<RenderTexturePtr>();

  // Send the texture to the Viewer
  if (current_texture != nullptr) {
    attached_viewer_->SetTexture(current_texture->texture());
  } else {
    attached_viewer_->SetTexture(0);
  }
}
