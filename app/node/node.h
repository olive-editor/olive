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

#ifndef NODE_H
#define NODE_H

#include <QCryptographicHash>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QXmlStreamWriter>

#include "codec/samplebuffer.h"
#include "common/rational.h"
#include "common/xmlutils.h"
#include "node/input.h"
#include "node/inputarray.h"
#include "node/output.h"
#include "node/value.h"
#include "render/audioparams.h"

OLIVE_NAMESPACE_ENTER

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
class Node : public QObject
{
  Q_OBJECT
public:
  enum Capabilities {
    kNormal = 0x0,
    kShader = 0x1,
    kSampleProcessor = 0x2
  };

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
   * @brief Clear current node variables and replace them with
   */
  void Load(QXmlStreamReader* reader, XMLNodeData &xml_node_data, const QAtomicInt *cancelled);

  /**
   * @brief Save this node into a text/XML format
   */
  void Save(QXmlStreamWriter* writer, const QString& custom_name = QString()) const;

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
  virtual QList<CategoryID> Category() const = 0;

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
   * @brief Return a list of NodeParams
   */
  const QList<NodeParam*>& parameters() const;

  /**
   * @brief Return the index of a parameter
   * @return Parameter index or -1 if this parameter is not part of this Node
   */
  int IndexOfParameter(NodeParam* param) const;

  /**
   * @brief Return a list of all Nodes that this Node's inputs are connected to (does not include this Node)
   */
  QList<Node*> GetDependencies() const;

  /**
   * @brief Returns a list of Nodes that this Node is dependent on, provided no other Nodes are dependent on them
   * outside of this hierarchy.
   *
   * Similar to GetDependencies(), but excludes any Nodes that are used outside the dependency graph of this Node.
   */
  QList<Node*> GetExclusiveDependencies() const;

  /**
   * @brief Retrieve immediate dependencies (only nodes that are directly connected to the inputs of this one)
   */
  QList<Node*> GetImmediateDependencies() const;

  /**
   * @brief Return accelerated capabilities of this node (if any)
   */
  virtual Capabilities GetCapabilities(const NodeValueDatabase&) const;

  /**
   * @brief Generate a unique identifier for the shader code (if a node can produce multiple)
   */
  virtual QString ShaderID(const NodeValueDatabase&) const;

  /**
   * @brief Generate hardware accelerated code for this Node
   */
  virtual QString ShaderVertexCode(const NodeValueDatabase&) const;

  /**
   * @brief Generate hardware accelerated code for this Node
   */
  virtual QString ShaderFragmentCode(const NodeValueDatabase&) const;

  /**
   * @brief Number of iterations to run the accelerated code
   *
   * Some code is faster if it's merely repeated on a resulting texture rather than run once on the same buffer.
   */
  virtual int ShaderIterations() const;

  /**
   * @brief Parameter that should receive the buffer on an iteration past the first
   */
  virtual NodeInput* ShaderIterativeInput() const;

  /**
   * @brief Return whether this node processes samples or not
   */
  virtual NodeInput* ProcessesSamplesFrom(const NodeValueDatabase &value) const;

  /**
   * @brief If ProcessesSamples() is true, this is the function that will process them.
   */
  virtual void ProcessSamples(const NodeValueDatabase &values, const AudioRenderingParams& params, const SampleBufferPtr input, SampleBufferPtr output, int index) const;

  /**
   * @brief Returns the input with the specified ID (or nullptr if it doesn't exist)
   */
  NodeInput* GetInputWithID(const QString& id) const;

  /**
   * @brief Returns the output with the specified ID (or nullptr if it doesn't exist)
   */
  NodeOutput* GetOutputWithID(const QString& id) const;

  /**
   * @brief Returns whether this Node outputs data to the Node `n` in any way
   */
  bool OutputsTo(Node* n) const;

  /**
   * @brief Return whether this Node has input parameters
   */
  bool HasInputs() const;

  /**
   * @brief Return whether this Node has output parameters
   */
  bool HasOutputs() const;

  /**
   * @brief Return whether this Node has input parameters and at least one of them is connected
   */
  bool HasConnectedInputs() const;

  /**
   * @brief Return whether this Node has output parameters and at least one of them is connected
   */
  bool HasConnectedOutputs() const;

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
  QList<TimeRange> TransformTimeTo(const TimeRange& time, Node* target, NodeParam::Type direction);

  template<class T>
  /**
   * @brief Find a node of a certain type that this Node outputs to
   */
  T* FindOutputNode();

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
  virtual void InvalidateCache(const TimeRange& range, NodeInput* from, NodeInput* source);

  /**
   * @brief Signal through node graph to only invalidate frames that are currently visible on a ViewerWidget
   */
  virtual void InvalidateVisible(NodeInput *from, NodeInput* source);

  /**
   * @brief Adjusts time that should be sent to nodes connected to certain inputs.
   *
   * If this node modifies the `time` (i.e. a clip converting sequence time to media time), this function should be
   * overridden to do so. Also make sure to override OutputTimeAdjustment() to provide the inverse function.
   */
  virtual TimeRange InputTimeAdjustment(NodeInput* input, const TimeRange& input_time) const;

  /**
   * @brief The inverse of InputTimeAdjustment()
   */
  virtual TimeRange OutputTimeAdjustment(NodeInput* input, const TimeRange& input_time) const;

  /**
   * @brief Copies inputs from from Node to another including connections
   *
   * Nodes must be of the same types (i.e. have the same ID)
   */
  static void CopyInputs(Node* source, Node* destination, bool include_connections = true);

  /**
   * @brief Return whether this Node can be deleted or not
   */
  bool CanBeDeleted() const;

  /**
   * @brief Set whether this Node can be deleted in the UI or not
   */
  void SetCanBeDeleted(bool s);

  /**
   * @brief Returns whether this Node is a "Block" type or not
   *
   * You shouldn't ever need to override this since all derivatives of Block will automatically have this set to true.
   * It's just a more convenient way of checking than dynamic_casting.
   */
  virtual bool IsBlock() const;

  /**
   * @brief Returns whether this Node is a "Track" type or not
   *
   * You shouldn't ever need to override this since all derivatives of Track will automatically have this set to true.
   * It's just a more convenient way of checking than dynamic_casting.
   */
  virtual bool IsTrack() const;

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

  NodeOutput* output() const;

  virtual NodeValue InputValueFromTable(NodeInput* input, NodeValueDatabase &db, bool take) const;

  const QPointF& GetPosition() const;

  void SetPosition(const QPointF& pos);

  static QString ReadFileAsString(const QString& filename);

  QList<NodeInput*> GetInputsIncludingArrays() const;

  QList<NodeOutput*> GetOutputs() const;

  virtual bool HasGizmos() const;

  virtual void DrawGizmos(const NodeValueDatabase& db, QPainter* p, const QVector2D &scale) const;

  virtual bool GizmoPress(const NodeValueDatabase& db, const QPointF& p, const QVector2D &scale);
  virtual void GizmoMove(const QPointF& p, const QVector2D &scale);
  virtual void GizmoRelease(const QPointF& p);

  const QString& GetLabel() const;
  void SetLabel(const QString& s);

  virtual void Hash(QCryptographicHash& hash, const rational &time) const;

protected:
  void AddInput(NodeInput* input);

  void ClearCachedValuesInParameters(const rational& start_range, const rational& end_range);

  void SendInvalidateCache(const TimeRange &range, NodeInput *source);

  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data);

  virtual void SaveInternal(QXmlStreamWriter* writer) const;

  virtual QList<NodeInput*> GetInputsToHash() const;

public slots:

signals:
  /**
   * @brief Signal emitted when a node is connected to another node (creating an "edge")
   *
   * @param edge
   *
   * The edge that was added
   */
  void EdgeAdded(NodeEdgePtr edge);

  /**
   * @brief Signal emitted when a node is disconnected from another node (removing an "edge")
   *
   * @param edge
   *
   * The edge that was removed
   */
  void EdgeRemoved(NodeEdgePtr edge);

  /**
   * @brief Signal emitted whenever the position is set through SetPosition()
   */
  void PositionChanged(const QPointF& pos);

  /**
   * @brief Signal emitted when SetLabel() is called
   */
  void LabelChanged(const QString& s);

private:
  /**
   * @brief Add a parameter to this node
   *
   * The Node takes ownership of this parameter.
   *
   * This can be either an output or an input at any time. Parameters will always appear in the order they're added.
   */
  void AddParameter(NodeParam* param);

  bool HasParamOfType(NodeParam::Type type, bool must_be_connected) const;

  void ConnectInput(NodeInput* input);

  void DisconnectInput(NodeInput* input);

  QList<Node *> GetDependenciesInternal(bool traverse, bool exclusive_only) const;

  QList<NodeParam *> params_;

  /**
   * @brief Internal variable for whether this Node can be deleted or not
   */
  bool can_be_deleted_;

  /**
   * @brief Primary node output
   */
  NodeOutput* output_;

  /**
   * @brief UI position for NodeViews
   */
  QPointF position_;

  /**
   * @brief Custom user label for node
   */
  QString label_;

private slots:
  void InputChanged(const OLIVE_NAMESPACE::TimeRange &range);

  void InputConnectionChanged(NodeEdgePtr edge);

};

template<class T>
T* Node::ValueToPtr(const QVariant &ptr)
{
  return reinterpret_cast<T*>(ptr.value<quintptr>());
}

template<class T>
Node* FindOutputNodeInternal(Node* n) {
  foreach (NodeEdgePtr edge, n->output()->edges()) {
    Node* connected = edge->input()->parentNode();
    T* cast_test = dynamic_cast<T*>(connected);

    if (cast_test) {
      return cast_test;
    } else {
      Node* drill_test = FindOutputNodeInternal<T>(connected);
      if (drill_test) {
        return drill_test;
      }
    }
  }

  return nullptr;
}

template<class T>
T* Node::FindOutputNode()
{
  return static_cast<T*>(FindOutputNodeInternal<T>(this));
}

OLIVE_NAMESPACE_EXIT

#endif // NODE_H
