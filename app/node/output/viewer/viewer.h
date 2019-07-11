#ifndef VIEWER_H
#define VIEWER_H

#include "node/node.h"

class ViewerOutput : public Node
{
public:
  ViewerOutput(QObject* parent = nullptr);

private:
  NodeInput* texture_input_;
};

#endif // VIEWER_H
