/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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
    extern QIcon LeftArrow;
    extern QIcon RightArrow;
    extern QIcon UpArrow;
    extern QIcon DownArrow;
    extern QIcon Diamond;
    extern QIcon Clock;

    extern QIcon MediaVideo;
    extern QIcon MediaAudio;
    extern QIcon MediaImage;
    extern QIcon MediaError;
    extern QIcon MediaSequence;
    extern QIcon MediaFolder;

    extern QIcon ViewerGoToStart;
    extern QIcon ViewerPrevFrame;
    extern QIcon ViewerPlay;
    extern QIcon ViewerPause;
    extern QIcon ViewerNextFrame;
    extern QIcon ViewerGoToEnd;

    void Initialize();

    /**
     * @brief Converts an SVG into a QIcon with a semi-transparent for the QIcon::Disabled property
     *
     * @param path
     *
     * Path to SVG file
     *
     * @param create_disabled
     *
     * Create a semi-transparent disabled option.
     */
    QIcon CreateIconFromSVG(const QString &path, bool create_disabled = true);
  }
}

#endif // ICONS_H
