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

#ifndef NODEFACTORY_H
#define NODEFACTORY_H

#include <QList>

#include "node.h"
#include "widget/menu/menu.h"

OLIVE_NAMESPACE_ENTER

class NodeFactory
{
public:
  enum InternalID {
    kViewerOutput,
    kClipBlock,
    kGapBlock,
    kAudioInput,
    kPolygonGenerator,
    kMatrixGenerator,
    kVideoInput,
    kTrackOutput,
    kAudioVolume,
    kAudioPanning,
    kMath,
    kTime,
    kTrigonometry,
    kBlurFilter,
    kSolidGenerator,
    kMerge,
    kStrokeFilter,

    // Count value
    kInternalNodeCount
  };

  NodeFactory() = default;

  static void Initialize();

  static void Destroy();

  static Menu* CreateMenu(QWidget *parent);

  static Node* CreateFromMenuAction(QAction* action);

  static Node* CreateFromID(const QString& id);

private:
  static Node* CreateInternal(const InternalID& id);

  static QList<Node*> library_;
};

OLIVE_NAMESPACE_EXIT

#endif // NODEFACTORY_H
