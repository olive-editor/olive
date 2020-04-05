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

#ifndef TOOL_H
#define TOOL_H

#include <QCoreApplication>
#include <QString>

class Tool {
public:
  /**
   * @brief A list of tools that can be used throughout the application
   */
  enum Item {
    /// No tool. This should never be set as the application tool, its only real purpose is to indicate the lack of
    /// a tool somewhere similar to nullptr.
    kNone,

    /// Pointer tool
    kPointer,

    /// Edit tool
    kEdit,

    /// Ripple tool
    kRipple,

    /// Rolling tool
    kRolling,

    /// Razor tool
    kRazor,

    /// Slip tool
    kSlip,

    /// Slide tool
    kSlide,

    /// Hand tool
    kHand,

    /// Zoom tool
    kZoom,

    /// Transition tool
    kTransition,

    /// Record tool
    kRecord,

    /// Add tool
    kAdd,

    kCount
  };

  /**
   * @brief Tools that can be added using the kAdd tool
   */
  enum AddableObject {
    /// An empty clip
    kAddableEmpty,

    /// A video clip showing a generic video placeholder
    kAddableBars,

    /// A video clip with a solid connected
    kAddableSolid,

    /// A video clip with a title connected
    kAddableTitle,

    /// An audio clip with a sine connected to it
    kAddableTone,

    kAddableCount
  };

  static QString GetAddableObjectName(const AddableObject& a)
  {
    switch (a) {
    case kAddableEmpty:
      return QCoreApplication::translate("Tool", "Empty");
    case kAddableBars:
      return QCoreApplication::translate("Tool", "Bars");
    case kAddableSolid:
      return QCoreApplication::translate("Tool", "Solid");
    case kAddableTitle:
      return QCoreApplication::translate("Tool", "Title");
    case kAddableTone:
      return QCoreApplication::translate("Tool", "Tone");
    case kAddableCount:
      break;
    }

    return QCoreApplication::translate("Tool", "Unknown");
  }

};

#endif // TOOL_H
