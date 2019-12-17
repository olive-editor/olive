#ifndef NODEFACTORY_H
#define NODEFACTORY_H

#include <QList>

#include "node.h"
#include "widget/menu/menu.h"

class NodeFactory
{
public:
  enum InternalID {
    kViewerOutput,
    kClipBlock,
    kGapBlock,
    kAudioInput,
    kTransformDistort,
    kVideoInput,
    kTimelineOutput,
    kTrackOutput,

    // Count value
    kInternalNodeCount
  };

  NodeFactory() = default;

  static void Initialize();

  static void Destroy();

  static Menu* CreateMenu();

  static Node* CreateFromMenuAction(QAction* action);

  static Node* CreateFromID(const QString& id);

private:
  static Node* CreateInternal(const InternalID& id);

  static QList<Node*> library_;
};

#endif // NODEFACTORY_H
