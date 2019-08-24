#ifndef RENDERERPROBE_H
#define RENDERERPROBE_H

#include "node/node.h"

class RendererProbe
{
public:
  RendererProbe();

  static void ProbeNode(Node* node, int thread_count, const rational &time);

private:
  static void TraverseNode(Node* node, int thread, const rational& time);
};

#endif // RENDERERPROBE_H
