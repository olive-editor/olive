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

#include "video.h"

#include <QDebug>
#include <QMatrix4x4>
#include <QOpenGLPixelTransferOptions>

#include "codec/ffmpeg/ffmpegdecoder.h"
#include "core.h"
#include "project/item/footage/footage.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

VideoInput::VideoInput()
{
  matrix_input_ = new NodeInput("matrix_in", NodeInput::kMatrix);
  AddInput(matrix_input_);
}

Node *VideoInput::copy() const
{
  return new VideoInput();
}

QString VideoInput::Name() const
{
  return tr("Video Input");
}

QString VideoInput::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.videoinput");
}

QString VideoInput::Description() const
{
  return tr("Import a video footage stream.");
}

NodeInput *VideoInput::matrix_input() const
{
  return matrix_input_;
}

Node::Capabilities VideoInput::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString VideoInput::ShaderVertexCode(const NodeValueDatabase&) const
{
  return ReadFileAsString(":/shaders/videoinput.vert");
}

QString VideoInput::ShaderFragmentCode(const NodeValueDatabase&) const
{
  return ReadFileAsString(":/shaders/videoinput.frag");
}

void VideoInput::Retranslate()
{
  MediaInput::Retranslate();

  matrix_input_->set_name(tr("Transform"));
}

OLIVE_NAMESPACE_EXIT
