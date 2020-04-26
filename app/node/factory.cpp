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
#include "block/transition/externaltransition.h"
#include "generator/matrix/matrix.h"
#include "input/media/video/video.h"
#include "input/media/audio/audio.h"
#include "input/time/timeinput.h"
#include "math/math/math.h"
#include "math/trigonometry/trigonometry.h"
#include "output/track/track.h"
#include "output/viewer/viewer.h"
#include "external.h"

OLIVE_NAMESPACE_ENTER
QList<Node*> NodeFactory::library_;

void NodeFactory::Initialize()
{
  Destroy();

  // Add internal types
  for (int i=0;i<kInternalNodeCount;i++) {
    library_.append(CreateInternal(static_cast<InternalID>(i)));
  }

  library_.append(new ExternalNode(":/shaders/blur.xml"));
  library_.append(new ExternalNode(":/shaders/solid.xml"));
  library_.append(new ExternalNode(":/shaders/stroke.xml"));
  library_.append(new ExternalNode(":/shaders/alphaover.xml"));
  library_.append(new ExternalNode(":/shaders/dropshadow.xml"));
  library_.append(new ExternalTransition(":/shaders/crossdissolve.xml"));
  library_.append(new ExternalTransition(":/shaders/diptoblack.xml"));
}

void NodeFactory::Destroy()
{
  qDeleteAll(library_);
  library_.clear();
}

Menu *NodeFactory::CreateMenu(QWidget* parent)
{
  Menu* menu = new Menu(parent);
  menu->setToolTipsVisible(true);

  for (int i=0;i<library_.size();i++) {
    Node* n = library_.at(i);

    // Make sure nodes are up-to-date with the current translation
    n->Retranslate();

    QStringList path = n->Category().split('/');

    Menu* destination = menu;

    // Find destination menu based on category hierarchy
    foreach (const QString& dir_name, path) {
      // Ignore an empty directory
      if (dir_name.isEmpty()) {
        continue;
      }

      // See if a menu with this dir_name already exists
      bool found_cat = false;
      QList<QAction*> menu_actions = destination->actions();
      foreach (QAction* action, menu_actions) {
        if (action->menu() && action->menu()->title() == dir_name) {
          destination = static_cast<Menu*>(action->menu());
          found_cat = true;
          break;
        }
      }

      // Create menu here if it doesn't exist
      if (!found_cat) {
        Menu* new_category = new Menu(dir_name, destination);
        destination->InsertAlphabetically(new_category);
        destination = new_category;
      }
    }

    // Add entry to menu
    QAction* a = destination->InsertAlphabetically(n->Name());
    a->setData(i);
    a->setToolTip(n->Description());
  }

  return menu;
}

Node* NodeFactory::CreateFromMenuAction(QAction *action)
{
  int library_index = action->data().toInt();

  if (library_index >= 0 && library_index < library_.size()) {
    return library_.at(library_index)->copy();
  }

  return nullptr;
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

Node *NodeFactory::CreateInternal(const NodeFactory::InternalID &id)
{
  switch (id) {
  case kClipBlock:
    return new ClipBlock();
  case kGapBlock:
    return new GapBlock();
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

  case kInternalNodeCount:
    break;
  }

  // We should never get here
  abort();
}

OLIVE_NAMESPACE_EXIT
