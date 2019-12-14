#include "factory.h"

#include "block/clip/clip.h"
#include "block/gap/gap.h"
#include "block/transition/crossdissolve/crossdissolve.h"
#include "distort/transform/transform.h"
#include "input/media/video/video.h"
#include "input/media/audio/audio.h"
#include "output/timeline/timeline.h"
#include "output/track/track.h"
#include "output/viewer/viewer.h"
#include "external.h"

QList<Node*> NodeFactory::library_;

void NodeFactory::Initialize()
{
  Destroy();

  // Add internal types
  for (int i=0;i<kInternalNodeCount;i++) {
    library_.append(CreateInternal(static_cast<InternalID>(i)));
  }

  library_.append(new ExternalNode(":/shaders/gaussianblur.xml"));
  library_.append(new ExternalNode(":/shaders/boxblur.xml"));
  library_.append(new ExternalNode(":/shaders/opacity.xml"));
  library_.append(new ExternalNode(":/shaders/solid.xml"));
  library_.append(new ExternalNode(":/shaders/alphaover.xml"));
}

void NodeFactory::Destroy()
{
  foreach (Node* n, library_) {
    delete n;
  }
  library_.clear();
}

Menu *NodeFactory::CreateMenu()
{
  Menu* menu = new Menu();
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
        Menu* new_category = new Menu(dir_name);
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
  case kTransitionBlock:
    return new CrossDissolveTransition();
  case kTransformDistort:
    return new TransformDistort();
  case kVideoInput:
    return new VideoInput();
  case kAudioInput:
    return new AudioInput();
  case kTimelineOutput:
    return new TimelineOutput();
  case kTrackOutput:
    return new TrackOutput();
  case kViewerOutput:
    return new ViewerOutput();

  case kInternalNodeCount:
    break;
  }

  // We should never get here
  abort();
}
