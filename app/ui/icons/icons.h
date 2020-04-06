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

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

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
extern QIcon ToolRolling;
extern QIcon ToolRazor;
extern QIcon ToolSlip;
extern QIcon ToolSlide;
extern QIcon ToolHand;
extern QIcon ToolTransition;

// Project Icons
extern QIcon Folder;
extern QIcon Sequence;
extern QIcon Video;
extern QIcon Audio;
extern QIcon Image;

// Triangle Arrows
extern QIcon TriUp;
extern QIcon TriLeft;
extern QIcon TriDown;
extern QIcon TriRight;

// Miscellaneous Icons
extern QIcon Snapping;
extern QIcon ZoomIn;
extern QIcon ZoomOut;
extern QIcon Record;
extern QIcon Add;
extern QIcon Error;
extern QIcon DirUp;
extern QIcon Clock;
extern QIcon Diamond;

/**
 * @brief Create an icon object loaded from file
 *
 * Using `name`, this function will load icon files to create an icon object that can be used throughout the
 * application.
 *
 * Olive's icons are stored in a very specific format. They are all sourced from SVGs, but stored as PNGs of various
 * sizes. See `app/ui/icons/genicons.sh`, as this script not only generates the multiple sizes but also the QRC file
 * used to compile the icons into the executable.
 *
 * This function is heavily tied into `genicons.sh` and will load all the different sized images (using the same
 * filename formatting and QRC resource directory) that `genicons.sh` generates into one QIcon file. If you change
 * either this function or `genicons.sh`, you will very likely have to change the other too.
 *
 * There is not much reason to call this outside of LoadAll() (which stores icons globally in memory so they don't
 * have to be reloaded each time a new object needs an icon).
 *
 * @param theme
 *
 * Name of the theme (used in the URL as the folder to load PNGs from)
 *
 * @param name
 *
 * Name of the icon (will correspond to the original SVG's filename with no path or extension)
 *
 * @return
 *
 * A QIcon object containing the various icon sizes loaded from resource
 */
QIcon Create(const QString &theme, const QString& name);

/**
 * @brief Methodically load all Olive icons into global variables that can be accessed throughout the application
 *
 * It's recommended to load any UI icons here so they're ready at startup and don't need to be re-loaded upon each
 * use.
 */
void LoadAll(const QString &theme);

}

OLIVE_NAMESPACE_EXIT

#endif // ICONS_H
