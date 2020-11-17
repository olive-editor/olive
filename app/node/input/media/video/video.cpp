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

#include "video.h"

#include <QDebug>
#include <QMatrix4x4>
#include <QOpenGLPixelTransferOptions>

#include "codec/ffmpeg/ffmpegdecoder.h"
#include "core.h"
#include "project/item/footage/footage.h"

OLIVE_NAMESPACE_ENTER

Node *VideoInput::copy() const
{
  return new VideoInput();
}

Stream::Type VideoInput::type() const
{
  return Stream::kVideo;
}

QString VideoInput::Name() const
{
  return tr("Video Input");
}

QString VideoInput::ShortName() const
{
  return tr("Video");
}

QString VideoInput::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.videoinput");
}

QString VideoInput::Description() const
{
  return tr("Import a video footage stream.");
}

OLIVE_NAMESPACE_EXIT
