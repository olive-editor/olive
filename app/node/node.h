/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef NODE_H
#define NODE_H

#include <QCryptographicHash>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QXmlStreamWriter>

#include "codec/frame.h"
#include "codec/samplebuffer.h"
#include "common/rational.h"
#include "common/xmlutils.h"
#include "node/connectable.h"
#include "node/input.h"
#include "node/value.h"
#include "render/audioparams.h"
#include "render/job/generatejob.h"
#include "render/job/samplejob.h"
#include "render/job/shaderjob.h"
#include "render/shadercode.h"

namespace olive {

class NodeGraph;

/**
 * @brief A single processing unit that can be connected with others to create intricate processing systems
 *
 * A cornerstone of "visual programming", a node is a single "function" that takes input and returns an output that can
 * be connected to other nodes. Inputs can be either user-set or retrieved from the output of another node. By joining
 * several nodes together, intricate, highly customizable, and infinitely extensible systems can be made for processing
 * data. It can also all be exposed to the user without forcing them to write code or compile anything.
 *
 * A major example in Olive is the entire rendering workflow. To render a frame, Olive will work through a node graph
 * that can be infinitely customized by the user to create images.
 *
 * This is a simple base class designed to contain all the functionality for this kind of processing connective unit.
 * It is an abstract class intended to be subclassed to create nodes with actual functionality.
 */
class Node : public NodeConnectable
{
  Q_OBJECT
public:
  enum CategoryID {
    kCategoryUnknown = -1,

    kCategoryInput,
    kCategoryOutput,
    kCategoryGenerator,
    kCategoryMath,
    kCategoryFilter,
    kCategoryColor,
    kCategoryGeneral,
    kCategoryTimeline,
    kCategoryChannels,
    kCategoryTransition,
    kCategoryDistort,

    kCategoryCount
  };

  Node();

  virtual ~Node() override;

  /**
   * @brief Creates a clone of the Node
   *
   * By default, the clone will NOT have the values and connections of the original node. The caller is responsible for
   * copying that data with functions like CopyInputs() as copies may be done for different reasons.
   */
  virtual Node* copy() const = 0;

  /**
   * @brief Convenience function - assumes parent is a NodeGraph
   */
  NodeGraph* parent() const;

  /**
   * @brief Clear current node variables and replace them with
   */
  void Load(QXmlStreamReader* reader, XMLNodeData &xml_node_data, const QAtomicInt *cancelled);

  /**
   * @brief Save this node into a text/XML format
   */
  void Save(QXmlStreamWriter* writer) const;

  /**
   * @brief Return the name of the node
   *
   * This is the node's name shown to the user. This must be overridden by subclasses, and preferably run through the
   * translator.
   */
  virtual QString Name() const = 0;

  /**
   * @brief Returns a shortened name of this node if applicable
   *
   * Defaults to returning Name() but can be overridden.
   */
  virtual QString ShortName() const;

  /**
   * @brief Return the unique identifier of the node
   *
   * This is used in save files and any other times a specific node must be picked out at runtime. This must be an ID
   * completely unique to this node, and preferably in bundle identifier format (e.g. "org.company.Name"). This string
   * should NOT be translated.
   */
  virtual QString id() const = 0;

  /**
   * @brief Return the category this node is in (optional for subclassing, but recommended)
   *
   * In any organized node menus, show the node in this category. If this node should be in a subfolder of a subfolder,
   * use a "/" to separate categories (e.g. "Distort/Noise"). The string should not start with a "/" as this will be
   * interpreted as an empty string category. This value should be run through a translator as its largely user
   * oriented.
   */
  virtual QVector<CategoryID> Category() const = 0;

  /**
   * @brief Return a description of this node's purpose (optional for subclassing, but recommended)
   *
   * A short (1-2 sentence) description of what this node should do to help the user understand its purpose. This should
   * be run through a translator.
   */
  virtual QString Description() const;

  /**
   * @brief Function called to retranslate parameter names (should be overridden in derivatives)
   */
  virtual void Retranslate();

  /**
   * @brief Retrieve the color of this node
   */
  Color color() const;

  /**
   * @brief Same as color() but return a pretty gradient version
   */
  QLinearGradient gradient_color(qreal top, qreal bottom) const;

  /**
   * @brief Uses config and returns either color() for flat shading or gradient for gradient
   */
  QBrush brush(qreal top, qreal bottom) const;

  /**
   * @brief Sets the override color. Set to -1 for no override color.
   */
  void SetOverrideColor(int index)
  {
    if (override_color_ != index) {
      override_color_ = index;
      emit ColorChanged();
    }
  }

  /**
   * @brief Return a list of NodeParams
   */
  const QVector<NodeInput*>& parameters() const
  {
    return inputs_;
  }

  const QVector<NodeInput*>& inputs() const
  {
    return inputs_;
  }

  /**
   * @brief Return the index of a parameter
   * @return Parameter index or -1 if this parameter is not part of this Node
   */
  int IndexOfParameter(NodeInput* param) const
  {
    return inputs_.indexOf(param);
  }

  /**
   * @brief Return a list of all Nodes that this Node's inputs are connected to (does not include this Node)
   */
  QVector<Node *> GetDependencies() const;

  /**
   * @brief Returns a list of Nodes that this Node is dependent on, provided no other Nodes are dependent on them
   * outside of this hierarchy.
   *
   * Similar to GetDependencies(), but excludes any Nodes that are used outside the dependency graph of this Node.
   */
  QVector<Node *> GetExclusiveDependencies() const;

  /**
   * @brief Retrieve immediate dependencies (only nodes that are directly connected to the inputs of this one)
   */
  QVector<Node *> GetImmediateDependencies() const;

  /**
   * @brief Generate hardware accelerated code for this Node
   */
  virtual ShaderCode GetShaderCode(const QString& shader_id) const;

  /**
   * @brief If Value() pushes a ShaderJob, this is the function that will process them.
   */
  virtual void ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const;

  /**
   * @brief If Value() pushes a GenerateJob, override this function for the image to create
   *
   * @param frame
   *
   * The destination buffer. It will already be allocated and ready for writing to.
   */
  virtual void GenerateFrame(FramePtr frame, const GenerateJob &job) const;

  /**
   * @brief Returns the input with the specified ID (or nullptr if it doesn't exist)
   */
  NodeInput* GetInputWithID(const QString& id) const;

  /**
   * @brief Returns whether this Node outputs to `n`
   *
   * @param n
   *
   * The node instance to check.
   *
   * @param recursively
   *
   * Whether to keep traversing down outputs to find this node (TRUE) or stick to immediate outputs
   * (FALSE).
   */
  bool OutputsTo(Node* n, bool recursively) const;

  /**
   * @brief Same as OutputsTo(Node*), but for a node ID rather than a specific instance.
   */
  bool OutputsTo(const QString& id, bool recursively) const;

  /**
   * @brief Same as OutputsTo(Node*), but for a specific node input rather than just a node.
   */
  bool OutputsTo(NodeInput* input, bool recursively) const;

  /**
   * @brief Returns whether this node ever receives an input from a particular node instance
   */
  bool InputsFrom(Node* n, bool recursively) const;

  /**
   * @brief Returns whether this node ever receives an input from a node with a particular ID
   */
  bool InputsFrom(const QString& id, bool recursively) const;

  /**
   * @brief Determines how many paths go from this node out to another node
   */
  int GetRoutesTo(Node* n) const;

  /**
   * @brief Severs all input and output connections
   */
  void DisconnectAll();

  /**
   * @brief Get the human-readable name for any category
   */
  static QString GetCategoryName(const CategoryID &c);

  /**
   * @brief Transforms time from this node through the connections it takes to get to the specified node
   */
  QVector<TimeRange> TransformTimeTo(const TimeRange& time, Node* target, bool input_dir);

  /**
   * @brief Find nodes of a certain type that this Node takes inputs from
   */
  template<class T>
  QVector<T*> FindInputNodes() const;

  template<class T>
  /**
   * @brief Find a node of a certain type that this Node outputs to
   */
  QVector<T *>  FindOutputNode();

  /**
   * @brief Convert a pointer to a value that can be sent between NodeParams
   */
  static QVariant PtrToValue(void* ptr);

  template<class T>
  /**
   * @brief Convert a NodeParam value to a pointer of any kind
   */
  static T* ValueToPtr(const QVariant& ptr);

  /**
   * @brief Signal all dependent Nodes that anything cached between start_range and end_range is now invalid and
   *        requires re-rendering
   *
   * Override this if your Node subclass keeps a cache, but call this base function at the end of the subclass function.
   * Default behavior is to relay this signal to all connected outputs, which will need to be done as to not break
   * the DAG. Even if the time needs to be transformed somehow (e.g. converting media time to sequence time), you can
   * call this function with transformed time and relay the signal that way.
   */
  virtual void InvalidateCache(const TimeRange& range, const InputConnection& from = InputConnection());

  /**
   * @brief Limits cache invalidation temporarily
   *
   * If you intend to do a number of operations in quick succession, you can optimize it by running
   * this function with EndOperation().
   */
  virtual void BeginOperation();

  /**
   * @brief Stops limiting cache invalidation and flushes changes
   */
  virtual void EndOperation();

  /**
   * @brief Adjusts time that should be sent to nodes connected to certain inputs.
   *
   * If this node modifies the `time` (i.e. a clip converting sequence time to media time), this function should be
   * overridden to do so. Also make sure to override OutputTimeAdjustment() to provide the inverse function.
   */
  virtual TimeRange InputTimeAdjustment(NodeInput* input, int element, const TimeRange& input_time) const;

  /**
   * @brief The inverse of InputTimeAdjustment()
   */
  virtual TimeRange OutputTimeAdjustment(NodeInput* input, int element, const TimeRange& input_time) const;

  /**
   * @brief Copies inputs from from Node to another including connections
   *
   * Nodes must be of the same types (i.e. have the same ID)
   */
  static void CopyInputs(Node* source, Node* destination, bool include_connections = true);

  /**
   * @brief Clones a set of nodes and connects the new ones the way the old ones were
   */
  static QVector<Node*> CopyDependencyGraph(const QVector<Node*>& nodes, QUndoCommand *command);
  static void CopyDependencyGraph(const QVector<Node*>& src, const QVector<Node*>& dst, QUndoCommand *command);

  /**
   * @brief Return whether this Node can be deleted or not
   */
  bool CanBeDeleted() const;

  /**
   * @brief Set whether this Node can be deleted in the UI or not
   */
  void SetCanBeDeleted(bool s);

  /**
   * @brief The main processing function
   *
   * The node's main purpose is to take values from inputs to set values in outputs. For whatever subclass node you
   * create, this is where the code for that goes.
   *
   * Note that as a video editor, the node graph has to work across time. Depending on the purpose of your node, it may
   * output different values depending on the time, and even if not, it will likely be receiving different input
   * depending on the time. Most of the difficult work here is handled by NodeInput::get_value() which you should pass
   * the `time` parameter to. It will return its value (at that time, if it's keyframed), or pass the time to a
   * corresponding output if it's connected to one. If your node doesn't directly deal with time, the default behavior
   * of the NodeParam objects will handle everything related to it automatically.
   */
  virtual NodeValueTable Value(NodeValueDatabase& value) const;

  /**
   * @brief Return whether a parameter with ID `id` has already been added to this Node
   */
  bool HasParamWithID(const QString& id) const;

  const QPointF& GetPosition() const;

  void SetPosition(const QPointF& pos);

  virtual bool HasGizmos() const;

  virtual void DrawGizmos(NodeValueDatabase& db, QPainter* p);

  virtual bool GizmoPress(NodeValueDatabase& db, const QPointF& p);
  virtual void GizmoMove(const QPointF& p, const rational &time);
  virtual void GizmoRelease();

  const QString& GetLabel() const;
  void SetLabel(const QString& s);

  virtual void Hash(QCryptographicHash& hash, const rational &time) const;

  const std::vector<InputConnection>& edges() const
  {
    return output_connections();
  }

  void InvalidateAll(NodeInput *input, int element);

protected:
  void SendInvalidateCache(const TimeRange &range);

  /**
   * @brief Don't send cache invalidation signals if `input` is connected or disconnected
   *
   * By default, when a node is connected or disconnected from input, the Node assumes that the
   * parameters has changed throughout the duration of the clip (essential from 0 to infinity).
   * In some scenarios, it may be preferable to handle this signal separately in order to
   */
  void IgnoreInvalidationsFrom(NodeInput* input);

  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data);

  virtual void SaveInternal(QXmlStreamWriter* writer) const;

  virtual QVector<NodeInput*> GetInputsToHash() const;

  enum GizmoScaleHandles {
    kGizmoScaleTopLeft,
    kGizmoScaleTopCenter,
    kGizmoScaleTopRight,
    kGizmoScaleBottomLeft,
    kGizmoScaleBottomCenter,
    kGizmoScaleBottomRight,
    kGizmoScaleCenterLeft,
    kGizmoScaleCenterRight,
    kGizmoScaleCount,
  };

  static QRectF CreateGizmoHandleRect(const QPointF& pt, int radius);

  static double GetGizmoHandleRadius(const QTransform& transform);

  static void DrawAndExpandGizmoHandles(QPainter* p, int handle_radius, QRectF* rects, int count);

  virtual void childEvent(QChildEvent* event) override;

signals:
  /**
   * @brief Signal emitted whenever the position is set through SetPosition()
   */
  void PositionChanged(const QPointF& pos);

  /**
   * @brief Signal emitted when SetLabel() is called
   */
  void LabelChanged(const QString& s);

  void ColorChanged();

  void ValueChanged(NodeInput* input, int element);

  void InputConnected(Node* output, NodeInput* input, int element);

  void InputDisconnected(Node* output, NodeInput* input, int element);

  void OutputConnected(NodeInput* destination, int element);

  void OutputDisconnected(NodeInput* destination, int element);

private:
  template<class T>
  static void FindInputNodeInternal(const Node* n, QVector<T *>& list);

  template<class T>
  static void FindOutputNodeInternal(const Node* n, QVector<T *>& list);

  QVector<Node*> GetDependenciesInternal(bool traverse, bool exclusive_only) const;

  void HashInputElement(QCryptographicHash& hash, NodeInput* input, int element, const rational& time) const;

  QVector<NodeInput*> inputs_;

  QVector<NodeInput*> ignore_connections_;

  /**
   * @brief Internal variable for whether this Node can be deleted or not
   */
  bool can_be_deleted_;

  /**
   * @brief UI position for NodeViews
   */
  QPointF position_;

  /**
   * @brief Custom user label for node
   */
  QString label_;

  /**
   * @brief -1 if the color should be based on the category, >=0 if the user has set a custom color
   */
  int override_color_;

private slots:
  void ParameterValueChanged(const olive::TimeRange &range, int element);

  void ParameterConnected(Node* source, int element);

  void ParameterDisconnected(Node* source, int element);

};

template<class T>
void Node::FindInputNodeInternal(const Node* n, QVector<T *> &list)
{
  foreach (NodeInput* input, n->inputs_) {
    for (auto it=input->edges().cbegin(); it!=input->edges().cend(); it++) {
      Node* edge = it->second;
      T* cast_test = dynamic_cast<T*>(edge);

      if (cast_test) {
        list.append(cast_test);
      }

      FindInputNodeInternal<T>(edge, list);
    }
  }
}

template<class T>
QVector<T *> Node::FindInputNodes() const
{
  QVector<T *> list;

  FindInputNodeInternal<T>(this, list);

  return list;
}

template<class T>
T* Node::ValueToPtr(const QVariant &ptr)
{
  return reinterpret_cast<T*>(ptr.value<quintptr>());
}

template<class T>
void Node::FindOutputNodeInternal(const Node* n, QVector<T *>& list)
{
  foreach (const InputConnection& edge, n->edges()) {
    Node* connected = static_cast<Node*>(edge.input->parent());
    T* cast_test = dynamic_cast<T*>(connected);

    if (cast_test) {
      list.append(cast_test);
    }

    FindOutputNodeInternal<T>(connected);
  }
}

template<class T>
QVector<T *> Node::FindOutputNode()
{
  QVector<T *> list;

  FindOutputNodeInternal<T>(this, list);

  return list;
}

using NodePtr = std::shared_ptr<Node>;

}

#endif // NODE_H
