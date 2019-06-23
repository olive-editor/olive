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

#ifndef ICONS_H
#define ICONS_H

#include <QIcon>

namespace olive {
namespace icon {

// Playback Icons
extern QIcon GoToStart;
extern QIcon PrevFrame;
extern QIcon Play;
extern QIcon Pause;
extern QIcon NextFrame;
extern QIcon GoToEnd;

// Project Management Toolbar Icons
extern QIcon New;
extern QIcon Open;
extern QIcon Save;
extern QIcon Undo;
extern QIcon Redo;
extern QIcon TreeView;
extern QIcon ListView;
extern QIcon IconView;

// Tool Icons
extern QIcon ToolPointer;
extern QIcon ToolEdit;
extern QIcon ToolRipple;
extern QIcon ToolRazor;
extern QIcon ToolSlip;
extern QIcon ToolSlide;
extern QIcon ToolHand;
extern QIcon ToolTransition;

// Miscellaneous Icons
extern QIcon Snapping;
extern QIcon ZoomIn;
extern QIcon ZoomOut;
extern QIcon Record;
extern QIcon Add;

QIcon Create(const QString& name);
void LoadAll();

}
}

#endif // ICONS_H
