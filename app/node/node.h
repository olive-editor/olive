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

#include <map>
#include <QCryptographicHash>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QXmlStreamWriter>

#include "codec/frame.h"
#include "codec/samplebuffer.h"
#include "common/rational.h"
#include "common/timerange.h"
#include "common/xmlutils.h"
#include "node/keyframe.h"
#include "node/inputimmediate.h"
#include "node/param.h"
#include "render/audioparams.h"
#include "render/job/generatejob.h"
#include "render/job/samplejob.h"
#include "render/job/shaderjob.h"
#include "render/shadercode.h"
#include "splitvalue.h"

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
class Node : public QObject
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
    kCategoryProject,

    kCategoryCount
  };

  Node(bool create_default_output = true);

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

  Project* project() const;

  /**
   * @brief Clear current node variables and replace them with
   */
  void Load(QXmlStreamReader* reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt *cancelled);

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

  virtual QIcon icon() const;

  const QVector<QString>& inputs() const
  {
    return input_ids_;
  }

  const QVector<QString>& outputs() const
  {
    return outputs_;
  }

  bool HasInputWithID(const QString& id) const
  {
    return input_ids_.contains(id);
  }

  bool HasOutputWithID(const QString& id) const
  {
    return outputs_.contains(id);
  }

  bool HasParamWithID(const QString& id) const
  {
    return HasInputWithID(id) || HasOutputWithID(id);
  }

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

  static void ConnectEdge(const NodeOutput& output, const NodeInput& input);

  static void DisconnectEdge(const NodeOutput& output, const NodeInput& input);

  QString GetInputName(const QString& id) const;

  void LoadInput(QXmlStreamReader* reader, XMLNodeData &xml_node_data, const QAtomicInt *cancelled);
  void SaveInput(QXmlStreamWriter* writer, const QString& id) const;

  bool IsInputConnectable(const QString& input) const;
  bool IsInputKeyframable(const QString& input) const;

  bool IsInputKeyframing(const QString& input, int element = -1) const;
  bool IsInputKeyframing(const NodeInput& input) const
  {
    return IsInputKeyframing(input.input(), input.element());
  }

  void SetInputIsKeyframing(const QString& input, bool e, int element = -1);
  void SetInputIsKeyframing(const NodeInput& input, bool e)
  {
    SetInputIsKeyframing(input.input(), e, input.element());
  }

  bool IsInputConnected(const QString& input, int element = -1) const;
  bool IsInputConnected(const NodeInput& input) const
  {
    return IsInputConnected(input.input(), input.element());
  }

  bool IsInputStatic(const QString& input, int element = -1) const
  {
    return !IsInputConnected(input, element) && !IsInputKeyframing(input, element);
  }

  bool IsInputStatic(const NodeInput& input) const
  {
    return IsInputStatic(input.input(), input.element());
  }

  NodeOutput GetConnectedOutput(const QString& input, int element = -1) const;

  NodeOutput GetConnectedOutput(const NodeInput& input) const
  {
    return GetConnectedOutput(input.input(), input.element());
  }

  Node* GetConnectedNode(const QString& input, int element = -1) const
  {
    return GetConnectedOutput(input, element).node();
  }

  Node* GetConnectedNode(const NodeInput& input) const
  {
    return GetConnectedNode(input.input(), input.element());
  }

  bool IsUsingStandardValue(const QString& input, int track, int element = -1) const;

  NodeValue::Type GetInputDataType(const QString& id) const;
  void SetInputDataType(const QString& id, const NodeValue::Type& type);

  bool HasInputProperty(const QString& id, const QString& name) const;
  QHash<QString, QVariant> GetInputProperties(const QString& id) const;
  QVariant GetInputProperty(const QString& id, const QString& name) const;
  void SetInputProperty(const QString& id, const QString& name, const QVariant& value);

  QVariant GetValueAtTime(const QString& input, const rational& time, int element = -1) const
  {
    NodeValue::Type type = GetInputDataType(input);

    return NodeValue::combine_track_values_into_normal_value(type, GetSplitValueAtTime(input, time, element));
  }

  QVariant GetValueAtTime(const NodeInput& input, const rational& time)
  {
    return GetValueAtTime(input.input(), time, input.element());
  }

  SplitValue GetSplitValueAtTime(const QString& input, const rational& time, int element = -1) const;

  SplitValue GetSplitValueAtTime(const NodeInput& input, const rational& time)
  {
    return GetSplitValueAtTime(input.input(), time, input.element());
  }

  QVariant GetSplitValueAtTimeOnTrack(const QString& input, const rational& time, int track, int element = -1) const;
  QVariant GetSplitValueAtTimeOnTrack(const NodeInput& input, const rational& time, int track) const
  {
    return GetSplitValueAtTimeOnTrack(input.input(), time, track, input.element());
  }

  QVariant GetSplitValueAtTimeOnTrack(const NodeKeyframeTrackReference& input, const rational& time) const
  {
    return GetSplitValueAtTimeOnTrack(input.input(), time, input.track());
  }

  QVariant GetDefaultValue(const QString& input) const;
  SplitValue GetSplitDefaultValue(const QString& input) const;
  QVariant GetSplitDefaultValueOnTrack(const QString& input, int track) const;

  const QVector<NodeKeyframeTrack>& GetKeyframeTracks(const QString& input, int element) const;
  const QVector<NodeKeyframeTrack>& GetKeyframeTracks(const NodeInput& input) const
  {
    return GetKeyframeTracks(input.input(), input.element());
  }

  QVector<NodeKeyframe*> GetKeyframesAtTime(const QString& input, const rational& time, int element = -1) const;
  QVector<NodeKeyframe*> GetKeyframesAtTime(const NodeInput& input, const rational& time) const
  {
    return GetKeyframesAtTime(input.input(), time, input.element());
  }

  NodeKeyframe* GetKeyframeAtTimeOnTrack(const QString& input, const rational& time, int track, int element = -1) const;
  NodeKeyframe* GetKeyframeAtTimeOnTrack(const NodeInput& input, const rational& time, int track) const
  {
    return GetKeyframeAtTimeOnTrack(input.input(), time, track, input.element());
  }

  NodeKeyframe* GetKeyframeAtTimeOnTrack(const NodeKeyframeTrackReference& input, const rational& time) const
  {
    return GetKeyframeAtTimeOnTrack(input.input(), time, input.track());
  }

  NodeKeyframe::Type GetBestKeyframeTypeForTimeOnTrack(const QString& input, const rational& time, int track, int element = -1) const;

  NodeKeyframe::Type GetBestKeyframeTypeForTimeOnTrack(const NodeInput& input, const rational& time, int track) const
  {
    return GetBestKeyframeTypeForTimeOnTrack(input.input(), time, track, input.element());
  }

  NodeKeyframe::Type GetBestKeyframeTypeForTimeOnTrack(const NodeKeyframeTrackReference& input, const rational& time) const
  {
    return GetBestKeyframeTypeForTimeOnTrack(input.input(), time, input.track());
  }

  int GetNumberOfKeyframeTracks(const QString& id) const;
  int GetNumberOfKeyframeTracks(const NodeInput& id) const
  {
    return GetNumberOfKeyframeTracks(id.input());
  }

  NodeKeyframe* GetEarliestKeyframe(const QString& id, int element = -1) const;
  NodeKeyframe* GetEarliestKeyframe(const NodeInput& id) const
  {
    return GetEarliestKeyframe(id.input(), id.element());
  }

  NodeKeyframe* GetLatestKeyframe(const QString& id, int element = -1) const;
  NodeKeyframe* GetLatestKeyframe(const NodeInput& id) const
  {
    return GetLatestKeyframe(id.input(), id.element());
  }

  NodeKeyframe* GetClosestKeyframeBeforeTime(const QString& id, const rational& time, int element = -1) const;
  NodeKeyframe* GetClosestKeyframeBeforeTime(const NodeInput& id, const rational& time) const
  {
    return GetClosestKeyframeBeforeTime(id.input(), time, id.element());
  }

  NodeKeyframe* GetClosestKeyframeAfterTime(const QString& id, const rational& time, int element = -1) const;
  NodeKeyframe* GetClosestKeyframeAfterTime(const NodeInput& id, const rational& time) const
  {
    return GetClosestKeyframeAfterTime(id.input(), time, id.element());
  }

  bool HasKeyframeAtTime(const QString& id, const rational& time, int element = -1) const;
  bool HasKeyframeAtTime(const NodeInput& id, const rational& time) const
  {
    return HasKeyframeAtTime(id.input(), time, id.element());
  }

  QStringList GetComboBoxStrings(const QString& id) const;

  QVariant GetStandardValue(const QString& id, int element = -1) const;
  QVariant GetStandardValue(const NodeInput& id) const
  {
    return GetStandardValue(id.input(), id.element());
  }

  SplitValue GetSplitStandardValue(const QString& id, int element = -1) const;
  SplitValue GetSplitStandardValue(const NodeInput& id) const
  {
    return GetSplitStandardValue(id.input(), id.element());
  }

  QVariant GetSplitStandardValueOnTrack(const QString& input, int track, int element = -1) const;
  QVariant GetSplitStandardValueOnTrack(const NodeKeyframeTrackReference& id) const
  {
    return GetSplitStandardValueOnTrack(id.input().input(), id.track(), id.input().element());
  }

  void SetStandardValue(const QString& id, const QVariant& value, int element = -1);
  void SetStandardValue(const NodeInput& id, const QVariant& value)
  {
    SetStandardValue(id.input(), value, id.element());
  }

  void SetSplitStandardValue(const QString& id, const SplitValue& value, int element = -1);
  void SetSplitStandardValue(const NodeInput& id, const SplitValue& value)
  {
    SetSplitStandardValue(id.input(), value, id.element());
  }

  void SetSplitStandardValueOnTrack(const QString& id, int track, const QVariant& value, int element = -1);
  void SetSplitStandardValueOnTrack(const NodeKeyframeTrackReference& id, const QVariant& value)
  {
    SetSplitStandardValueOnTrack(id.input().input(), id.track(), value, id.input().element());
  }

  bool InputIsArray(const QString& id) const;

  void InputArrayInsert(const QString& id, int index, bool undoable = false);
  void InputArrayResize(const QString& id, int size, bool undoable = false);
  void InputArrayRemove(const QString& id, int index, bool undoable = false);

  void InputArrayAppend(const QString& id, bool undoable = false)
  {
    InputArrayResize(id, InputArraySize(id) + 1, undoable);
  }

  void InputArrayPrepend(const QString& id, bool undoable = false)
  {
    InputArrayInsert(id, 0, undoable);
  }

  void InputArrayRemoveLast(const QString& id, bool undoable = false)
  {
    InputArrayResize(id, InputArraySize(id) - 1, undoable);
  }

  int InputArraySize(const QString& id) const;

  const NodeKeyframeTrack& GetTrackFromKeyframe(NodeKeyframe* key) const;

  using InputConnections = std::map<NodeInput, NodeOutput>;

  /**
   * @brief Return map of input connections
   *
   * Inputs can only have one connection, so the key is the input connected and the value is the
   * output that it's connected to.
   */
  const InputConnections& input_connections() const
  {
    return input_connections_;
  }

  using OutputConnection = std::pair<NodeOutput, NodeInput>;
  using OutputConnections = std::vector<OutputConnection>;

  /**
   * @brief Return list of output connections
   *
   * An output can connect an infinite amount of inputs, so in this map, the key is the output and
   * the value is a vector of inputs.
   */
  const OutputConnections& output_connections() const
  {
    return output_connections_;
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
  bool OutputsTo(const NodeInput &input, bool recursively) const;

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
  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element, qint64 job_time);

  void InvalidateCache(const TimeRange& range, const QString& from, int element = -1)
  {
    InvalidateCache(range, from, element, last_change_time_);
  }

  void InvalidateCache(const TimeRange& range, const NodeInput& from)
  {
    InvalidateCache(range, from.input(), from.element());
  }

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
  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const;

  /**
   * @brief The inverse of InputTimeAdjustment()
   */
  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const;

  /**
   * @brief Copies inputs from from Node to another including connections
   *
   * Nodes must be of the same types (i.e. have the same ID)
   */
  static void CopyInputs(const Node *source, Node* destination, bool include_connections = true);

  static void CopyInput(const Node *src, Node* dst, const QString& input, bool include_connections, bool traverse_arrays);

  static void CopyValuesOfElement(const Node* src, Node* dst, const QString& input, int src_element, int dst_element);
  static void CopyValuesOfElement(const Node* src, Node* dst, const QString& input, int element)
  {
    return CopyValuesOfElement(src, dst, input, element, element);
  }

  /**
   * @brief Clones a set of nodes and connects the new ones the way the old ones were
   */
  static QVector<Node*> CopyDependencyGraph(const QVector<Node*>& nodes, MultiUndoCommand *command);
  static void CopyDependencyGraph(const QVector<Node*>& src, const QVector<Node*>& dst, MultiUndoCommand *command);

  static Node* CopyNodeAndDependencyGraphMinusItems(const Node* node, MultiUndoCommand* command);

  static Node* CopyNodeInGraph(const Node* node, MultiUndoCommand* command);

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
  virtual NodeValueTable Value(const QString &output, NodeValueDatabase& value) const;

  const QPointF& GetPosition() const;

  void SetPosition(const QPointF& pos);

  virtual bool HasGizmos() const;

  virtual void DrawGizmos(NodeValueDatabase& db, QPainter* p);

  virtual bool GizmoPress(NodeValueDatabase& db, const QPointF& p);
  virtual void GizmoMove(const QPointF& p, const rational &time);
  virtual void GizmoRelease();

  const QString& GetLabel() const;
  void SetLabel(const QString& s);

  virtual void Hash(const QString& output, QCryptographicHash& hash, const rational &time) const;

  void InvalidateAll(const QString& input, int element = -1);

  bool HasLinks() const
  {
    return !links_.isEmpty();
  }

  const QVector<Node*>& links() const
  {
    return links_;
  }

  static bool Link(Node* a, Node* b);
  static bool Unlink(Node* a, Node* b);
  static bool AreLinked(Node* a, Node* b);

  static const QString kDefaultOutput;

protected:
  enum InputFlag {
    /// By default, inputs are keyframable, connectable, and NOT arrays
    kInputFlagNormal = 0x0,
    kInputFlagArray = 0x1,
    kInputFlagNotKeyframable = 0x2,
    kInputFlagNotConnectable = 0x4
  };

  class InputFlags {
  public:
    explicit InputFlags()
    {
      f_ = kInputFlagNormal;
    }

    explicit InputFlags(uint64_t flags)
    {
      f_ = flags;
    }

    bool operator&(const InputFlag& f) const
    {
      return f_ & f;
    }

  private:
    uint64_t f_;

  };

  void AddInput(const QString& id, NodeValue::Type type, const QVariant& default_value, InputFlags flags = InputFlags(kInputFlagNormal));
  void AddInput(const QString& id, NodeValue::Type type, InputFlags flags = InputFlags(kInputFlagNormal))
  {
    AddInput(id, type, QVariant(), flags);
  }

  void RemoveInput(const QString& id);

  void AddOutput(const QString& id = kDefaultOutput);

  void RemoveOutput(const QString& id);

  void SetInputName(const QString& id, const QString& name);

  void SetComboBoxStrings(const QString& id, const QStringList& strings)
  {
    SetInputProperty(id, QStringLiteral("combo_str"), strings);
  }

  void SendInvalidateCache(const TimeRange &range, qint64 job_time);

  /**
   * @brief Don't send cache invalidation signals if `input` is connected or disconnected
   *
   * By default, when a node is connected or disconnected from input, the Node assumes that the
   * parameters has changed throughout the duration of the clip (essential from 0 to infinity).
   * In some scenarios, it may be preferable to handle this signal separately in order to
   */
  void IgnoreInvalidationsFrom(const QString &input_id);

  void IgnoreHashingFrom(const QString& input_id);

  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data, uint version, const QAtomicInt* cancelled);

  virtual void SaveInternal(QXmlStreamWriter* writer) const;

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

  virtual void LinkChangeEvent(){}

  virtual void InputValueChangedEvent(const QString& input, int element);

  virtual void InputConnectedEvent(const QString& input, int element, const NodeOutput& output);

  virtual void InputDisconnectedEvent(const QString& input, int element, const NodeOutput& output);

  virtual void OutputConnectedEvent(const QString& output, const NodeInput& input);

  virtual void OutputDisconnectedEvent(const QString& output, const NodeInput& input);

  virtual void childEvent(QChildEvent *event) override;

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

  void ValueChanged(const NodeInput& input, const TimeRange& range);

  void InputConnected(const NodeOutput& output, const NodeInput& input);

  void InputDisconnected(const NodeOutput& output, const NodeInput& input);

  void OutputConnected(const NodeOutput& output, const NodeInput& input);

  void OutputDisconnected(const NodeOutput& output, const NodeInput& input);

  void InputPropertyChanged(const QString& input, const QString& key, const QVariant& value);

  void LinksChanged();

  void InputArraySizeChanged(const QString& input, int new_size);

  void KeyframeAdded(NodeKeyframe* key);

  void KeyframeRemoved(NodeKeyframe* key);

  void KeyframeTimeChanged();

  void KeyframeEnableChanged(const NodeInput& input, bool enabled);

  void InputAdded(const QString& id);

  void InputRemoved(const QString& id);

  void OutputAdded(const QString& id);

  void OutputRemoved(const QString& id);

  void InputNameChanged(const QString& id, const QString& name);

  void InputDataTypeChanged(const QString& id, NodeValue::Type type);

private:
  class ArrayInsertCommand : public UndoCommand
  {
  public:
    ArrayInsertCommand(Node* node, const QString& input, int index) :
      node_(node),
      input_(input),
      index_(index)
    {
    }

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override
    {
      node_->InputArrayInsert(input_, index_, false);
    }

    virtual void undo() override
    {
      node_->InputArrayRemove(input_, index_, false);
    }

  private:
    Node* node_;
    QString input_;
    int index_;

  };

  class ArrayRemoveCommand : public UndoCommand
  {
  public:
    ArrayRemoveCommand(Node* node, const QString& input, int index) :
      node_(node),
      input_(input),
      index_(index)
    {
    }

    virtual Project* GetRelevantProject() const override;

  protected:
    virtual void redo() override
    {
      // Save immediate data
      if (node_->IsInputKeyframable(input_)) {
        is_keyframing_ = node_->IsInputKeyframing(input_, index_);
      }
      standard_value_ = node_->GetSplitStandardValue(input_, index_);
      keyframes_ = node_->GetKeyframeTracks(input_, index_);
      node_->GetImmediate(input_, index_)->delete_all_keyframes(&memory_manager_);

      node_->InputArrayRemove(input_, index_, false);
    }

    virtual void undo() override
    {
      node_->InputArrayInsert(input_, index_, false);

      // Restore keyframes
      foreach (const NodeKeyframeTrack& track, keyframes_) {
        foreach (NodeKeyframe* key, track) {
          key->setParent(node_);
        }
      }
      node_->SetSplitStandardValue(input_, standard_value_, index_);

      if (node_->IsInputKeyframable(input_)) {
        node_->SetInputIsKeyframing(input_, is_keyframing_, index_);
      }
    }

  private:
    Node* node_;
    QString input_;
    int index_;

    SplitValue standard_value_;
    bool is_keyframing_;
    QVector<NodeKeyframeTrack> keyframes_;
    QObject memory_manager_;

  };

  class ArrayResizeCommand : public UndoCommand
  {
  public:
    ArrayResizeCommand(Node* node, const QString& input, int size) :
      node_(node),
      input_(input),
      size_(size)
    {}

    virtual void redo() override
    {
      old_size_ = node_->InputArraySize(input_);

      if (old_size_ > size_) {
        // Decreasing in size, disconnect any extraneous edges
        for (int i=size_; i<old_size_; i++) {

          try {
            NodeInput input(node_, input_, i);
            NodeOutput output = node_->input_connections().at(input);

            removed_connections_[input] = output;

            DisconnectEdge(output, input);
          } catch (std::out_of_range&) {}
        }
      }

      node_->ArrayResizeInternal(input_, size_);
    }

    virtual void undo() override
    {
      for (auto it=removed_connections_.cbegin(); it!=removed_connections_.cend(); it++) {
        ConnectEdge(it->second, it->first);
      }
      removed_connections_.clear();

      node_->ArrayResizeInternal(input_, old_size_);
    }

    virtual Project* GetRelevantProject() const override;

  private:
    Node* node_;
    QString input_;
    int size_;
    int old_size_;

    InputConnections removed_connections_;

  };

  struct Input {
    NodeValue::Type type;
    InputFlags flags;
    SplitValue default_value;
    QHash<QString, QVariant> properties;
    QString human_name;
    int array_size;
  };

  NodeInputImmediate* CreateImmediate(const QString& input);

  NodeInputImmediate* GetImmediate(const QString& input, int element) const;

  int GetInternalInputIndex(const QString& input) const
  {
    return input_ids_.indexOf(input);
  }

  InputFlags GetInputFlags(const QString& input) const;

  Input* GetInternalInputData(const QString& input)
  {
    int i = GetInternalInputIndex(input);

    if (i == -1) {
      return nullptr;
    } else {
      return &input_data_[i];
    }
  }

  const Input* GetInternalInputData(const QString& input) const
  {
    int i = GetInternalInputIndex(input);

    if (i == -1) {
      return nullptr;
    } else {
      return &input_data_.at(i);
    }
  }

  void ReportInvalidInput(const char* attempted_action, const QString &id) const;

  void ArrayResizeInternal(const QString& id, int size);

  static Node *CopyNodeAndDependencyGraphMinusItemsInternal(QMap<const Node*, Node*>& created, const Node *node, MultiUndoCommand *command);

  /**
   * @brief Immediates aren't deleted, so the actual array size may be larger than ArraySize()
   */
  int GetInternalInputArraySize(const QString& input);

  template<class T>
  static void FindInputNodeInternal(const Node* n, QVector<T *>& list);

  template<class T>
  static void FindOutputNodeInternal(const Node* n, QVector<T *>& list);

  QVector<Node*> GetDependenciesInternal(bool traverse, bool exclusive_only) const;

  void HashInputElement(QCryptographicHash& hash, const QString &input, int element, const rational& time) const;

  void ParameterValueChanged(const QString &input, int element, const olive::TimeRange &range);
  void ParameterValueChanged(const NodeInput& input, const olive::TimeRange &range)
  {
    ParameterValueChanged(input.input(), input.element(), range);
  }

  void LoadImmediate(QXmlStreamReader *reader, const QString& input, int element, XMLNodeData& xml_node_data, const QAtomicInt* cancelled);

  void SaveImmediate(QXmlStreamWriter *writer, const QString &input, int element) const;

  void UpdateLastChangedTime();

  /**
   * @brief Intelligently determine how what time range is affected by a keyframe
   */
  TimeRange GetRangeAffectedByKeyframe(NodeKeyframe *key) const;

  /**
   * @brief Gets a time range between the previous and next keyframes of index
   */
  TimeRange GetRangeAroundIndex(const QString& input, int index, int track, int element) const;

  void ClearElement(const QString &input, int index);

  QVector<QString> ignore_connections_;

  QVector<QString> ignore_when_hashing_;

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

  /**
   * @brief Nodes that are linked with this one
   */
  QVector<Node*> links_;

  QVector<QString> input_ids_;
  QVector<Input> input_data_;

  QVector<QString> outputs_;

  QMap<QString, NodeInputImmediate*> standard_immediates_;

  QMap<QString, QVector<NodeInputImmediate*> > array_immediates_;

  InputConnections input_connections_;

  OutputConnections output_connections_;

  qint64 last_change_time_;

private slots:
  /**
   * @brief Slot when a keyframe's time changes to keep the keyframes correctly sorted by time
   */
  void InvalidateFromKeyframeTimeChange();

  /**
   * @brief Slot when a keyframe's value changes to signal that the cache needs updating
   */
  void InvalidateFromKeyframeValueChange();

  /**
   * @brief Slot when a keyframe's type changes to signal that the cache needs updating
   */
  void InvalidateFromKeyframeTypeChanged();

  /**
   * @brief Slot when a keyframe's bezier in value changes to signal that the cache needs updating
   */
  void InvalidateFromKeyframeBezierInChange();

  /**
   * @brief Slot when a keyframe's bezier out value changes to signal that the cache needs updating
   */
  void InvalidateFromKeyframeBezierOutChange();

};

template<class T>
void Node::FindInputNodeInternal(const Node* n, QVector<T *> &list)
{
  for (auto it=n->input_connections_.cbegin(); it!=n->input_connections_.cend(); it++) {
    Node* edge = it->second.node();
    T* cast_test = dynamic_cast<T*>(edge);

    if (cast_test) {
      list.append(cast_test);
    }

    FindInputNodeInternal<T>(edge, list);
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
  foreach (const OutputConnection& output, n->output_connections_) {
    Node* connected = output.second.node();
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
