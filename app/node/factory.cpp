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

#include "factory.h"

#include "audio/pan/pan.h"
#include "audio/volume/volume.h"
#include "block/clip/clip.h"
#include "block/gap/gap.h"
#include "block/transition/crossdissolve/crossdissolvetransition.h"
#include "block/transition/diptocolor/diptocolortransition.h"
#include "generator/matrix/matrix.h"
#include "generator/polygon/polygon.h"
#include "generator/solid/solid.h"
#include "generator/text/text.h"
#include "filter/blur/blur.h"
#include "filter/color/color.h"
#include "filter/stroke/stroke.h"
#include "input/media/video/video.h"
#include "input/media/audio/audio.h"
#include "input/time/timeinput.h"
#include "math/math/math.h"
#include "math/merge/merge.h"
#include "math/trigonometry/trigonometry.h"
#include "output/track/track.h"
#include "output/viewer/viewer.h"

OLIVE_NAMESPACE_ENTER
QList<Node*> NodeFactory::library_;

void NodeFactory::Initialize()
{
  Destroy();

  // Add internal types
  for (int i=0;i<kInternalNodeCount;i++) {
    library_.append(CreateFromFactoryIndex(static_cast<InternalID>(i)));
  }

  /*
  library_.append(new ExternalTransition(":/shaders/crossdissolve.xml"));
  library_.append(new ExternalTransition(":/shaders/diptoblack.xml"));
  */
}

void NodeFactory::Destroy()
{
  qDeleteAll(library_);
  library_.clear();
}

Menu *NodeFactory::CreateMenu(QWidget* parent, bool create_none_item, Node::CategoryID restrict_to)
{
  Menu* menu = new Menu(parent);
  menu->setToolTipsVisible(true);

  for (int i=0;i<library_.size();i++) {
    Node* n = library_.at(i);

    if (restrict_to != Node::kCategoryUnknown && !n->Category().contains(restrict_to)) {
      // Skip this node
      continue;
    }

    // Make sure nodes are up-to-date with the current translation
    n->Retranslate();

    Menu* destination = nullptr;

    QString category_name = Node::GetCategoryName(n->Category().isEmpty()
                                                  ? Node::kCategoryUnknown
                                                  : n->Category().first());

    // See if a menu with this category name already exists
    QList<QAction*> menu_actions = menu->actions();
    foreach (QAction* action, menu_actions) {
      if (action->menu() && action->menu()->title() == category_name) {
        destination = static_cast<Menu*>(action->menu());
        break;
      }
    }

    // Create menu here if it doesn't exist
    if (!destination) {
      destination = new Menu(category_name, menu);
      menu->InsertAlphabetically(destination);
    }

    // Add entry to menu
    QAction* a = destination->InsertAlphabetically(n->Name());
    a->setData(i);
    a->setToolTip(n->Description());
  }

  if (create_none_item) {

    QAction* none_item = new QAction(QCoreApplication::translate("NodeFactory", "None"),
                                     menu);

    none_item->setData(-1);

    if (menu->actions().isEmpty()) {
      menu->addAction(none_item);
    } else {
      QAction* separator = menu->insertSeparator(menu->actions().first());
      menu->insertAction(separator, none_item);
    }
  }

  return menu;
}

Node* NodeFactory::CreateFromMenuAction(QAction *action)
{
  int index = action->data().toInt();

  if (index == -1) {
    return nullptr;
  }

  return library_.at(index)->copy();
}

QString NodeFactory::GetIDFromMenuAction(QAction *action)
{
  int index = action->data().toInt();

  if (index == -1) {
    return QString();
  }

  return library_.at(action->data().toInt())->id();
}

QString NodeFactory::GetNameFromID(const QString &id)
{
  if (!id.isEmpty()) {
    foreach (Node* n, library_) {
      if (n->id() == id) {
        return n->Name();
      }
    }
  }

  return QString();
}

Node *NodeFactory::CreateFromID(const QString &id)
{
  foreach (Node* n, library_) {
    if (n->id() == id) {
      return n->copy();
    }
  }

  return nullptr;
}

Node *NodeFactory::CreateFromFactoryIndex(const NodeFactory::InternalID &id)
{
  switch (id) {
  case kClipBlock:
    return new ClipBlock();
  case kGapBlock:
    return new GapBlock();
  case kPolygonGenerator:
    return new PolygonGenerator();
  case kMatrixGenerator:
    return new MatrixGenerator();
  case kVideoInput:
    return new VideoInput();
  case kAudioInput:
    return new AudioInput();
  case kTrackOutput:
    return new TrackOutput();
  case kViewerOutput:
    return new ViewerOutput();
  case kAudioVolume:
    return new VolumeNode();
  case kAudioPanning:
    return new PanNode();
  case kMath:
    return new MathNode();
  case kTrigonometry:
    return new TrigonometryNode();
  case kTime:
    return new TimeInput();
  case kBlurFilter:
    return new BlurFilterNode();
  case kColorFilter:
    return new ColorFilterNode();
  case kSolidGenerator:
    return new SolidGenerator();
  case kMerge:
    return new MergeNode();
  case kStrokeFilter:
    return new StrokeFilterNode();
  case kTextGenerator:
    return new TextGenerator();
  case kCrossDissolveTransition:
    return new CrossDissolveTransition();
  case kDipToColorTransition:
    return new DipToColorTransition();

  case kInternalNodeCount:
    break;
  }

  // We should never get here
  abort();
}

OLIVE_NAMESPACE_EXIT
