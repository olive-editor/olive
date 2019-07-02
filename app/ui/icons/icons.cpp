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
QIcon olive::icon::ToolRazor;
QIcon olive::icon::ToolSlip;
QIcon olive::icon::ToolSlide;
QIcon olive::icon::ToolHand;
QIcon olive::icon::ToolTransition;
QIcon olive::icon::Folder;
QIcon olive::icon::Video;
QIcon olive::icon::Audio;
QIcon olive::icon::Image;
QIcon olive::icon::Snapping;
QIcon olive::icon::ZoomIn;
QIcon olive::icon::ZoomOut;
QIcon olive::icon::Record;
QIcon olive::icon::Add;
QIcon olive::icon::Error;

void olive::icon::LoadAll()
{
  GoToStart = Create("prev");
  PrevFrame = Create("rew");
  Play = Create("play");
  Pause = Create("pause");
  NextFrame = Create("ff");
  GoToEnd = Create("next");

  New = Create("new");
  Open = Create("open");
  Save = Create("save");
  Undo = Create("undo");
  Redo = Create("redo");
  TreeView = Create("treeview");
  ListView = Create("listview");
  IconView = Create("iconview");

  ToolPointer = Create("arrow");
  ToolEdit = Create("beam");
  ToolRipple = Create("ripple");
  ToolRazor = Create("razor");
  ToolSlip = Create("slip");
  ToolSlide = Create("slide");
  ToolHand = Create("hand");
  ToolTransition = Create("transition-tool");

  Folder = Create("folder");
  Video = Create("videosource");
  Audio = Create("audiosource");
  Image = Create("imagesource");

  Snapping = Create("magnet");
  ZoomIn = Create("zoomin");
  ZoomOut = Create("zoomout");
  Record = Create("record");
  Add = Create("add-button");
  Error = Create("error");
}

QIcon olive::icon::Create(const QString &name)
{
  QIcon icon;

  for (int i=0;i<ICON_SIZE_COUNT;i++) {
    icon.addFile(QString(":/icons/%1.%2.png").arg(name, QString::number(ICON_SIZES[i])),
                 QSize(ICON_SIZES[i], ICON_SIZES[i]));
  }

  return icon;
}
