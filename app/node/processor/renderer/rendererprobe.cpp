#include "rendererprobe.h"

#include <QDebug>

RendererProbe::RendererProbe()
{

}

void RendererProbe::ProbeNode(Node *node, int thread_count, const rational& time)
{
  Q_ASSERT(thread_count > 0);

  //QVector< QVector<Node*> > dependency_graph;

  //dependency_graph.resize(thread_count);

  TraverseNode(node, 0, time);
}

void RendererProbe::TraverseNode(Node *node, int thread, const rational& time)
{
  qDebug() << node << "will run on thread" << thread;

  QList<Node*> deps = node->GetImmediateDependenciesAt(time);

  foreach (Node* dep, deps) {
    qDebug() << "  Dependency found:" << dep;
  }

  foreach (Node* dep, deps) {
    TraverseNode(dep, thread, time);
    thread++;
  }
}
