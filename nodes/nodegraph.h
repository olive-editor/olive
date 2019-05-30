#ifndef NODEGRAPH_H
#define NODEGRAPH_H

#include <QVector>
#include <QObject>

#include "nodes/node.h"
#include "rendering/imagecache.h"

class NodeGraph : public QObject
{
  Q_OBJECT
public:
  NodeGraph();

  const int& width();
  void set_width(const int& w);

  const int& height();
  void set_height(const int& h);

  /**
   * @brief Process the graph
   *
   * Using the output node set by SetOutputNode(), this function will work backwards and perform every action in the
   * node hierarchy necessary to eventually process the output node. After this function returns, the output node should
   * contain valid values ready for accessing.
   *
   * Each node will cache its output for optimized playback and rendering. This will happen transparently and under most
   * circumstances, this function will be able to return immediately.
   */
  void Process();

  /**
   * @brief Set the output node for this node graph
   *
   * This node will be presented as the final node that all other nodes converge on. This node can be used to retrieve
   * the result of the graph.
   *
   * @param node
   *
   * The node to set as the output node. The graph takes ownership of the node and the user cannot delete it.
   */
  void SetOutputNode(Node* node);

  /**
   * @brief Returns the currently set output node
   */
  Node* OutputNode();

  const rational &Time();
  void SetTime(const rational& d);

  ImageCache* memory_cache();

  QOpenGLContext* GLContext();
  void SetGLContext(QOpenGLContext* ctx);

signals:
  void NodeGraphChanged();
  void TimeChanged();
  void ParametersChanged();

protected:
  virtual void childEvent(QChildEvent *event) override;

private:  
  Node* output_node_;

  ImageCache memory_cache_;
  QOpenGLContext* ctx_;

  int width_;
  int height_;

  rational time_;
};

#endif // NODEGRAPH_H
