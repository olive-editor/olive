/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

namespace olive {

/// Works in conjunction with `genicons.sh` to generate and utilize icons of specific sizes
const int ICON_SIZE_COUNT = 4;
const int ICON_SIZES[] = {
  16,
  32,
  64,
  128
};

/// Internal icon library for use throughout Olive without having to regenerate constantly
QIcon icon::GoToStart;
QIcon icon::PrevFrame;
QIcon icon::Play;
QIcon icon::Pause;
QIcon icon::NextFrame;
QIcon icon::GoToEnd;
QIcon icon::New;
QIcon icon::Open;
QIcon icon::Save;
QIcon icon::Undo;
QIcon icon::Redo;
QIcon icon::TreeView;
QIcon icon::ListView;
QIcon icon::IconView;
QIcon icon::ToolPointer;
QIcon icon::ToolEdit;
QIcon icon::ToolRipple;
QIcon icon::ToolRolling;
QIcon icon::ToolRazor;
QIcon icon::ToolSlip;
QIcon icon::ToolSlide;
QIcon icon::ToolHand;
QIcon icon::ToolTransition;
QIcon icon::ToolTrackSelect;
QIcon icon::Folder;
QIcon icon::Sequence;
QIcon icon::Video;
QIcon icon::Audio;
QIcon icon::Image;
QIcon icon::MiniMap;
QIcon icon::TriUp;
QIcon icon::TriLeft;
QIcon icon::TriDown;
QIcon icon::TriRight;
QIcon icon::TextBold;
QIcon icon::TextItalic;
QIcon icon::TextUnderline;
QIcon icon::TextStrikethrough;
QIcon icon::TextSmallCaps;
QIcon icon::TextAlignLeft;
QIcon icon::TextAlignRight;
QIcon icon::TextAlignCenter;
QIcon icon::TextAlignJustify;
QIcon icon::TextAlignTop;
QIcon icon::TextAlignBottom;
QIcon icon::TextAlignMiddle;
QIcon icon::Snapping;
QIcon icon::ZoomIn;
QIcon icon::ZoomOut;
QIcon icon::Record;
QIcon icon::Add;
QIcon icon::Error;
QIcon icon::DirUp;
QIcon icon::Clock;
QIcon icon::Diamond;
QIcon icon::Plus;
QIcon icon::Minus;
QIcon icon::AddEffect;
QIcon icon::EyeOpened;
QIcon icon::EyeClosed;
QIcon icon::LockOpened;
QIcon icon::LockClosed;
QIcon icon::Pencil;
QIcon icon::Subtitles;
QIcon icon::ColorPicker;

void icon::LoadAll(const QString& theme)
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
  ToolTrackSelect = Create(theme, "track-tool");

  Folder = Create(theme, "folder");
  Sequence = Create(theme, "sequence");
  Video = Create(theme, "videosource");
  Audio = Create(theme, "audiosource");
  Image = Create(theme, "imagesource");

  MiniMap = Create(theme, "map");

  TriUp = Create(theme, "tri-up");
  TriLeft = Create(theme, "tri-left");
  TriDown = Create(theme, "tri-down");
  TriRight = Create(theme, "tri-right");

  TextBold = Create(theme, "text-bold");
  TextItalic = Create(theme, "text-italic");
  TextUnderline = Create(theme, "text-underline");
  TextStrikethrough = Create(theme, "text-strikethrough");
  TextSmallCaps = Create(theme, "text-small-caps");
  TextAlignLeft = Create(theme, "align-left");
  TextAlignRight = Create(theme, "align-right");
  TextAlignCenter = Create(theme, "align-center");
  TextAlignJustify = Create(theme, "align-justify-all");
  TextAlignTop = Create(theme, "align-v-top");
  TextAlignBottom = Create(theme, "align-v-bottom");
  TextAlignMiddle = Create(theme, "align-v-middle");

  Snapping = Create(theme, "magnet");
  ZoomIn = Create(theme, "zoomin");
  ZoomOut = Create(theme, "zoomout");
  Record = Create(theme, "record");
  Add = Create(theme, "add-button");
  Error = Create(theme, "error");
  DirUp = Create(theme, "dirup");
  Clock = Create(theme, "clock");
  Diamond = Create(theme, "diamond");
  Plus = Create(theme, "plus");
  Minus = Create(theme, "minus");
  AddEffect = Create(theme, "add-effect");
  ColorPicker = Create(theme, "color-picker");

  EyeOpened = Create(theme, "eye-opened");
  EyeClosed = Create(theme, "eye-closed");
  LockOpened = Create(theme, "lock-opened");
  LockClosed = Create(theme, "lock-closed");

  Pencil = Create(theme, "text-edit");
  Subtitles = Create(theme, "subtitles");
}

QIcon icon::Create(const QString& theme, const QString &name)
{
  QIcon icon;

  for (int i=0;i<ICON_SIZE_COUNT;i++) {
    icon.addFile(QStringLiteral("%1/png/%2.%3.png").arg(theme, name, QString::number(ICON_SIZES[i])),
                 QSize(ICON_SIZES[i], ICON_SIZES[i]), QIcon::Normal);
    icon.addFile(QStringLiteral("%1/png/%2.%3.disabled.png").arg(theme, name, QString::number(ICON_SIZES[i])),
                 QSize(ICON_SIZES[i], ICON_SIZES[i]), QIcon::Disabled);
  }

  return icon;
}

}
