#ifndef NODEFACTORY_H
#define NODEFACTORY_H

#include <QList>

#include "node.h"
#include "widget/menu/menu.h"

class NodeFactory
{
public:
  enum InternalID {
    kAlphaOverBlend,
    kClipBlock,
    kGapBlock,
    kTransitionBlock,
    kOpacity,
    kTransformDistort,
    kSolidGenerator,
    kVideoInput,
    kAudioInput,
    kTimelineOutput,
    kTrackOutput,
    kViewerOutput,

    // Count value
    kInternalNodeCount
  };

  NodeFactory() = default;

  static void Initialize();

  static void Destroy();

  static void Create(const Entry& create_info);

  static Menu* CreateMenu();

private:
  static Node* CreateInternal(const InternalID& id);

  static QList<Node*> library_;
};

#endif // NODEFACTORY_H
