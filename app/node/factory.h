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

#ifndef NODEFACTORY_H
#define NODEFACTORY_H

#include <QList>

#include "node.h"
#include "widget/menu/menu.h"

namespace olive {

class NodeFactory
{
public:
  enum InternalID {
    kViewerOutput,
    kClipBlock,
    kGapBlock,
    kPolygonGenerator,
    kMatrixGenerator,
    kTransformDistort,
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
    kTextGeneratorV1,
    kTextGeneratorV2,
    kTextGeneratorV3,
    kCrossDissolveTransition,
    kDipToColorTransition,
    kMosaicFilter,
    kCropDistort,
    kProjectFootage,
    kProjectFolder,
    kProjectSequence,
    kValueNode,
    kTimeRemapNode,
    kSubtitleBlock,
    kShapeGenerator,
    kColorDifferenceKeyKeying,
    kDespillKeying,
    kGroupNode,
    kOpacityEffect,
    kFlipDistort,
    kNoiseGenerator,
    kTimeOffsetNode,
    kCornerPinDistort,
    kDisplayTransform,
    kOCIOGradingTransformLinear,
    kChromaKey,
    kMaskDistort,
    kDropShadowFilter,
    kTimeFormat,
    kWaveDistort,
    kRippleDistort,
    kTileDistort,
    kSwirlDistort,
    kMulticamNode,

    // Count value
    kInternalNodeCount
  };

  NodeFactory() = default;

  static void Initialize();

  static void Destroy();

  static Menu* CreateMenu(QWidget *parent, bool create_none_item = false, Node::CategoryID restrict_to = Node::kCategoryUnknown, uint64_t restrict_flags = 0);

  static Node* CreateFromMenuAction(QAction* action);

  static QString GetIDFromMenuAction(QAction* action);

  static QString GetNameFromID(const QString& id);

  static Node* CreateFromID(const QString& id);

  static Node* CreateFromFactoryIndex(const InternalID& id);

private:
  static QList<Node*> library_;

};

}

#endif // NODEFACTORY_H
