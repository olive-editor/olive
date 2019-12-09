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

#include "icons.h"

/// Works in conjunction with `genicons.sh` to generate and utilize icons of specific sizes
const int ICON_SIZE_COUNT = 4;
const int ICON_SIZES[] = {
  16,
  32,
  64,
  128
};

/// Internal icon library for use throughout Olive without having to regenerate constantly
QIcon olive::icon::GoToStart;
QIcon olive::icon::PrevFrame;
QIcon olive::icon::Play;
QIcon olive::icon::Pause;
QIcon olive::icon::NextFrame;
QIcon olive::icon::GoToEnd;
QIcon olive::icon::New;
QIcon olive::icon::Open;
QIcon olive::icon::Save;
QIcon olive::icon::Undo;
QIcon olive::icon::Redo;
QIcon olive::icon::TreeView;
QIcon olive::icon::ListView;
QIcon olive::icon::IconView;
QIcon olive::icon::ToolPointer;
QIcon olive::icon::ToolEdit;
QIcon olive::icon::ToolRipple;
QIcon olive::icon::ToolRolling;
QIcon olive::icon::ToolRazor;
QIcon olive::icon::ToolSlip;
QIcon olive::icon::ToolSlide;
QIcon olive::icon::ToolHand;
QIcon olive::icon::ToolTransition;
QIcon olive::icon::Folder;
QIcon olive::icon::Sequence;
QIcon olive::icon::Video;
QIcon olive::icon::Audio;
QIcon olive::icon::Image;
QIcon olive::icon::TriUp;
QIcon olive::icon::TriLeft;
QIcon olive::icon::TriDown;
QIcon olive::icon::TriRight;
QIcon olive::icon::Snapping;
QIcon olive::icon::ZoomIn;
QIcon olive::icon::ZoomOut;
QIcon olive::icon::Record;
QIcon olive::icon::Add;
QIcon olive::icon::Error;
QIcon olive::icon::DirUp;

void olive::icon::LoadAll(const QString& theme)
{
  GoToStart = Create(theme, "prev");
  PrevFrame = Create(theme, "rew");
  Play = Create(theme, "play");
  Pause = Create(theme, "pause");
  NextFrame = Create(theme, "ff");
  GoToEnd = Create(theme, "next");

  New = Create(theme, "new");
  Open = Create(theme, "open");
  Save = Create(theme, "save");
  Undo = Create(theme, "undo");
  Redo = Create(theme, "redo");
  TreeView = Create(theme, "treeview");
  ListView = Create(theme, "listview");
  IconView = Create(theme, "iconview");

  ToolPointer = Create(theme, "arrow");
  ToolEdit = Create(theme, "beam");
  ToolRipple = Create(theme, "ripple");
  ToolRolling = Create(theme, "rolling");
  ToolRazor = Create(theme, "razor");
  ToolSlip = Create(theme, "slip");
  ToolSlide = Create(theme, "slide");
  ToolHand = Create(theme, "hand");
  ToolTransition = Create(theme, "transition-tool");

  Folder = Create(theme, "folder");
  Sequence = Create(theme, "sequence");
  Video = Create(theme, "videosource");
  Audio = Create(theme, "audiosource");
  Image = Create(theme, "imagesource");

  TriUp = Create(theme, "tri-up");
  TriLeft = Create(theme, "tri-left");
  TriDown = Create(theme, "tri-down");
  TriRight = Create(theme, "tri-right");

  Snapping = Create(theme, "magnet");
  ZoomIn = Create(theme, "zoomin");
  ZoomOut = Create(theme, "zoomout");
  Record = Create(theme, "record");
  Add = Create(theme, "add-button");
  Error = Create(theme, "error");
  DirUp = Create(theme, "dirup");
}

QIcon olive::icon::Create(const QString& theme, const QString &name)
{
  QIcon icon;

  for (int i=0;i<ICON_SIZE_COUNT;i++) {
    icon.addFile(QStringLiteral("%1/png/%2.%3.png").arg(theme, name, QString::number(ICON_SIZES[i])),
                 QSize(ICON_SIZES[i], ICON_SIZES[i]));
  }

  return icon;
}
