/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QXmlStreamWriter>

#include "codec/frame.h"
#include "common/xmlutils.h"
#include "node/gizmo/draggable.h"
#include "node/globals.h"
#include "node/keyframe.h"
#include "node/inputimmediate.h"
#include "node/param.h"
#include "node/swizzlemap.h"
#include "render/audioplaybackcache.h"
#include "render/audiowaveformcache.h"
#include "render/framehashcache.h"
#include "render/job/colortransformjob.h"
#include "render/job/generatejob.h"
#include "render/job/samplejob.h"
#include "render/job/shaderjob.h"
#include "widget/nodeparamview/paramwidget/abstractparamwidget.h"

namespace olive {

#define NODE_DEFAULT_FUNCTIONS(x) \
  NODE_DEFAULT_DESTRUCTOR(x) \
  NODE_COPY_FUNCTION(x)

#define NODE_DEFAULT_DESTRUCTOR(x) \
  virtual ~x() override {DisconnectAll();}

#define NODE_COPY_FUNCTION(x) \
  virtual Node *copy() const override {return new x();}

class Folder;
class Project;
struct SerializedData;

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

    kCategoryOutput,
    kCategoryGenerator,
    kCategoryMath,
    kCategoryKeying,
    kCategoryFilter,
    kCategoryColor,
    kCategoryTime,
    kCategoryTimeline,
    kCategoryTransition,
    kCategoryDistort,
    kCategoryProject,

    kCategoryCount
  };

  enum Flag {
    kNone = 0,
    kDontShowInParamView = 0x1,
    kVideoEffect = 0x2,
    kAudioEffect = 0x4,
    kDontShowInCreateMenu = 0x8,
    kIsItem = 0x10
  };

  static constexpr type_t TYPE_VEC2 = "vec2";
  static constexpr type_t TYPE_VEC3 = "vec3";
  static constexpr type_t TYPE_VEC4 = "vec4";
  static constexpr type_t TYPE_BOOL = "bool";
  static constexpr type_t TYPE_COLOR = "color";
  static constexpr type_t TYPE_COMBO = "combo";
  static constexpr type_t TYPE_FONT = "font";
  static constexpr type_t TYPE_FILE = "file";

  struct ContextPair {
    Node *node;
    Node *context;
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
  Project *parent() const;

  Project* project() const;

  const uint64_t &GetFlags() const
  {
    return flags_;
  }

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

  Folder* folder() const
  {
    return folder_;
  }

  bool IsItem() const { return flags_ & kIsItem; }

  /**
   * @brief Function called to retranslate parameter names (should be overridden in derivatives)
   */
  virtual void Retranslate();

  enum DataType
  {
    ICON,
    DURATION,
    CREATED_TIME,
    MODIFIED_TIME,
    FREQUENCY_RATE,
    TOOLTIP
  };

  virtual QVariant data(const DataType &d) const;

  const QVector<QString>& inputs() const { return input_ids_; }

  struct Output
  {
    QString id;
    QString name;
  };
  const QVector<Output>& outputs() const { return outputs_; }

  bool HasInputWithID(const QString& id) const { return input_ids_.contains(id); }

  // Node caches
  FrameHashCache* video_frame_cache() const { return video_cache_; }
  ThumbnailCache* thumbnail_cache() const { return thumbnail_cache_; }
  AudioPlaybackCache* audio_playback_cache() const { return audio_cache_; }
  AudioWaveformCache* waveform_cache() const { return waveform_cache_; }

  virtual TimeRange GetVideoCacheRange() const { return TimeRange(); }
  virtual TimeRange GetAudioCacheRange() const { return TimeRange(); }

  struct Position
  {
    Position(const QPointF &p = QPointF(0, 0), bool e = false)
    {
      position = p;
      expanded = e;
    }

    bool load(QXmlStreamReader *reader);
    void save(QXmlStreamWriter *writer) const;

    QPointF position;
    bool expanded;

    inline Position &operator+=(const Position &p)
    {
      position += p.position;
      return *this;
    }

    inline Position &operator-=(const Position &p)
    {
      position -= p.position;
      return *this;
    }

    friend inline const Position operator+(Position a, const Position &b)
    {
      a += b;
      return a;
    }

    friend inline const Position operator-(Position a, const Position &b)
    {
      a -= b;
      return a;
    }
  };

  using PositionMap = QHash<Node*, Position>;
  const PositionMap &GetContextPositions() const
  {
    return context_positions_;
  }

  bool IsNodeExpandedInContext(Node *node) const
  {
    return context_positions_.value(node).expanded;
  }

  bool ContextContainsNode(Node *node) const
  {
    return context_positions_.contains(node);
  }

  Position GetNodePositionDataInContext(Node *node)
  {
    return context_positions_.value(node);
  }

  QPointF GetNodePositionInContext(Node *node)
  {
    return GetNodePositionDataInContext(node).position;
  }

  bool SetNodePositionInContext(Node *node, const QPointF &pos);

  bool SetNodePositionInContext(Node *node, const Position &pos);

  void SetNodeExpandedInContext(Node *node, bool e)
  {
    context_positions_[node].expanded = e;
  }

  bool RemoveNodeFromContext(Node *node);

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

  int GetOverrideColor() const
  {
    return override_color_;
  }

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

  static bool ConnectionExists(const NodeOutput &output, const NodeInput& input);

  static void ConnectEdge(const NodeOutput &output, const NodeInput& input);

  static void DisconnectEdge(const NodeOutput &output, const NodeInput& input);

  void CopyCacheUuidsFrom(Node *n);

  bool AreCachesEnabled() const { return caches_enabled_; }
  void SetCachesEnabled(bool e) { caches_enabled_ = e; }

  virtual QString GetInputName(const QString& id) const;

  void SetInputName(const QString& id, const QString& name);
  void SetOutputName(const QString &id, const QString &name);

  bool IsInputHidden(const QString& input) const;
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

  NodeOutput GetConnectedOutput2(const QString& input, int element = -1) const;
  NodeOutput GetConnectedOutput2(const NodeInput& input) const
  {
    return GetConnectedOutput2(input.input(), input.element());
  }

  Node *GetConnectedOutput(const QString& input, int element = -1) const
  {
    return GetConnectedOutput2(input, element).node();
  }

  Node *GetConnectedOutput(const NodeInput& input) const
  {
    return GetConnectedOutput(input.input(), input.element());
  }

  std::vector<NodeOutput> GetConnectedOutputs(const QString &input, int element = -1) const;
  std::vector<NodeOutput> GetConnectedOutputs(const NodeInput &input) const
  {
    return GetConnectedOutputs(input.input(), input.element());
  }

  bool is_enabled() const { return GetStandardValue(kEnabledInput).toBool(); }
  void set_enabled(bool e) {  SetStandardValue(kEnabledInput, e); }

  bool IsUsingStandardValue(const QString& input, int track, int element = -1) const;

  type_t GetInputDataType(const QString& id) const;
  void SetInputDataType(const QString& id, const type_t& type, size_t channels = 1);

  bool HasInputProperty(const QString& id, const QString& name) const;
  QHash<QString, value_t> GetInputProperties(const QString& id) const;
  value_t GetInputProperty(const QString& id, const QString& name) const;
  void SetInputProperty(const QString& id, const QString& name, const value_t &value);

  value_t GetValueAtTime(const QString& input, const rational& time, int element = -1) const;
  value_t GetValueAtTime(const NodeInput& input, const rational& time)
  {
    return GetValueAtTime(input.input(), time, input.element());
  }

  value_t::component_t GetSplitValueAtTimeOnTrack(const QString& input, const rational& time, int track, int element = -1) const;
  value_t::component_t GetSplitValueAtTimeOnTrack(const NodeInput& input, const rational& time, int track) const
  {
    return GetSplitValueAtTimeOnTrack(input.input(), time, track, input.element());
  }

  value_t::component_t GetSplitValueAtTimeOnTrack(const NodeKeyframeTrackReference& input, const rational& time) const
  {
    return GetSplitValueAtTimeOnTrack(input.input(), time, input.track());
  }

  value_t GetDefaultValue(const QString& input) const;
  value_t::component_t GetSplitDefaultValueOnTrack(const QString& input, size_t track) const;

  void SetDefaultValue(const QString& input, const value_t &val);
  void SetSplitDefaultValueOnTrack(const QString& input, const value_t::component_t &val, size_t track);

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

  size_t GetNumberOfKeyframeTracks(const QString& id) const;
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

  value_t GetStandardValue(const QString& id, int element = -1) const;
  value_t GetStandardValue(const NodeInput& id) const
  {
    return GetStandardValue(id.input(), id.element());
  }

  value_t::component_t GetSplitStandardValueOnTrack(const QString& input, int track, int element = -1) const;
  value_t::component_t GetSplitStandardValueOnTrack(const NodeKeyframeTrackReference& id) const
  {
    return GetSplitStandardValueOnTrack(id.input().input(), id.track(), id.input().element());
  }

  void SetStandardValue(const QString& id, const value_t& value, int element = -1);
  void SetStandardValue(const NodeInput& id, const value_t& value)
  {
    SetStandardValue(id.input(), value, id.element());
  }

  void SetSplitStandardValueOnTrack(const QString& id, int track, const value_t::component_t& value, int element = -1);
  void SetSplitStandardValueOnTrack(const NodeKeyframeTrackReference& id, const value_t::component_t& value)
  {
    SetSplitStandardValueOnTrack(id.input().input(), id.track(), value, id.input().element());
  }

  bool InputIsArray(const QString& id) const;

  void InputArrayInsert(const QString& id, int index);
  void InputArrayResize(const QString& id, int size);
  void InputArrayRemove(const QString& id, int index);

  void InputArrayAppend(const QString& id)
  {
    InputArrayResize(id, InputArraySize(id) + 1);
  }

  void InputArrayPrepend(const QString& id)
  {
    InputArrayInsert(id, 0);
  }

  void InputArrayRemoveLast(const QString& id)
  {
    InputArrayResize(id, InputArraySize(id) - 1);
  }

  int InputArraySize(const QString& id) const;

  value_t GetInputValue(const ValueParams &g, const QString &input, int element = -1, bool autoconversion = true) const;
  value_t GetFakeConnectedValue(const ValueParams &g, NodeOutput output, const QString &input, int element = -1, bool autoconversion = true) const;

  NodeInputImmediate* GetImmediate(const QString& input, int element) const;

  NodeInput GetEffectInput()
  {
    return effect_input_.isEmpty() ? NodeInput() : NodeInput(this, effect_input_);
  }

  const QString &GetEffectInputID() const
  {
    return effect_input_;
  }

  class ValueHint
  {
  public:
    ValueHint() = default;

    const SwizzleMap &swizzle() const { return swizzle_; }
    void set_swizzle(const SwizzleMap &m) { swizzle_ = m; }

    bool load(QXmlStreamReader *reader);
    void save(QXmlStreamWriter *writer) const;

  private:
    SwizzleMap swizzle_;

  };

  const QMap<InputElementPair, ValueHint> &GetValueHints() const
  {
    return value_hints_;
  }

  ValueHint GetValueHintForInput(const QString &input, int element = -1) const
  {
    return value_hints_.value({input, element});
  }

  void SetValueHintForInput(const QString &input, const ValueHint &hint, int element = -1);

  virtual AbstractParamWidget *GetCustomWidget(const QString &input) const;

  const NodeKeyframeTrack& GetTrackFromKeyframe(NodeKeyframe* key) const;

  using Connection = std::pair<NodeOutput, NodeInput>;
  using Connections = std::vector<Connection>;

  /**
   * @brief Return map of input connections
   *
   * Inputs can only have one connection, so the key is the input connected and the value is the
   * output that it's connected to.
   */
  const Connections& input_connections() const
  {
    return input_connections_;
  }

  /**
   * @brief Return list of output connections
   *
   * An output can connect an infinite amount of inputs, so in this map, the key is the output and
   * the value is a vector of inputs.
   */
  const Connections& output_connections() const
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
   * @brief Returns whether this node ever receives an input from a particular node instance
   */
  bool InputsFrom(Node* n, bool recursively) const;

  /**
   * @brief Returns whether this node ever receives an input from a node with a particular ID
   */
  bool InputsFrom(const QString& id, bool recursively) const;

  /**
   * @brief Find inputs that `output` outputs to in order to arrive at this node
   *
   * Traverse this node's inputs recursively looking for `output`, and return a list of
   * edges that `output` uses to get to `this` node.
   */
  QVector<NodeInput> FindWaysNodeArrivesHere(const Node *output) const;

  /**
   * @brief Severs all input and output connections
   */
  void DisconnectAll();

  /**
   * @brief Get the human-readable name for any category
   */
  static QString GetCategoryName(const CategoryID &c);

  enum TransformTimeDirection {
    kTransformTowardsInput,
    kTransformTowardsOutput
  };

  /**
   * @brief Transforms time from this node through the connections it takes to get to the specified node
   */
  TimeRange TransformTimeTo(TimeRange time, Node* target, TransformTimeDirection dir, int path_index);

  /**
   * @brief Find nodes of a certain type that this Node takes inputs from
   */
  template<class T>
  QVector<T*> FindInputNodes(int maximum = 0) const;

  /**
   * @brief Find nodes of a certain type that this Node takes inputs from
   */
  template<class T>
  static QVector<T*> FindInputNodesConnectedToInput(const NodeInput &input, int maximum = 0);

  using InvalidateCacheOptions = QHash<QString, QVariant>;

  /**
   * @brief Signal all dependent Nodes that anything cached between start_range and end_range is now invalid and
   *        requires re-rendering
   *
   * Override this if your Node subclass keeps a cache, but call this base function at the end of the subclass function.
   * Default behavior is to relay this signal to all connected outputs, which will need to be done as to not break
   * the DAG. Even if the time needs to be transformed somehow (e.g. converting media time to sequence time), you can
   * call this function with transformed time and relay the signal that way.
   */
  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element = -1, InvalidateCacheOptions options = InvalidateCacheOptions());

  void InvalidateCache(const TimeRange& range, const NodeInput& from, const InvalidateCacheOptions &options = InvalidateCacheOptions())
  {
    InvalidateCache(range, from.input(), from.element(), options);
  }

  /**
   * @brief Adjusts time that should be sent to nodes connected to certain inputs.
   *
   * If this node modifies the `time` (i.e. a clip converting sequence time to media time), this function should be
   * overridden to do so. Also make sure to override OutputTimeAdjustment() to provide the inverse function.
   */
  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time, bool clamp) const;

  /**
   * @brief The inverse of InputTimeAdjustment()
   */
  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const;

  /**
   * @brief Copies inputs from from Node to another including connections
   *
   * Nodes must be of the same types (i.e. have the same ID)
   */
  static void CopyInputs(const Node *source, Node* destination, bool include_connections = true, MultiUndoCommand *command = nullptr);

  static void CopyInput(const Node *src, Node* dst, const QString& input, bool include_connections, bool traverse_arrays, MultiUndoCommand *command);

  static void CopyValuesOfElement(const Node* src, Node* dst, const QString& input, int src_element, int dst_element, MultiUndoCommand *command = nullptr);
  static void CopyValuesOfElement(const Node* src, Node* dst, const QString& input, int element, MultiUndoCommand *command = nullptr)
  {
    return CopyValuesOfElement(src, dst, input, element, element, command);
  }

  /**
   * @brief Clones a set of nodes and connects the new ones the way the old ones were
   */
  static QVector<Node*> CopyDependencyGraph(const QVector<Node*>& nodes, MultiUndoCommand *command);
  static void CopyDependencyGraph(const QVector<Node*>& src, const QVector<Node*>& dst, MultiUndoCommand *command);

  static Node* CopyNodeAndDependencyGraphMinusItems(Node* node, MultiUndoCommand* command);

  static Node* CopyNodeInGraph(Node *node, MultiUndoCommand* command);

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
  virtual value_t Value(const ValueParams &p) const {return value_t();}

  bool HasGizmos() const
  {
    return !gizmos_.isEmpty();
  }

  const QVector<NodeGizmo*> &GetGizmos() const
  {
    return gizmos_;
  }

  virtual QTransform GizmoTransformation(const ValueParams &p) const { return QTransform(); }

  virtual void UpdateGizmoPositions(const ValueParams &p){}

  const QString& GetLabel() const;
  void SetLabel(const QString& s);

  QString GetLabelAndName() const;
  QString GetLabelOrName() const;

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

  bool Load(QXmlStreamReader *reader, SerializedData *data);
  void Save(QXmlStreamWriter *writer) const;

  virtual bool LoadCustom(QXmlStreamReader *reader, SerializedData *data);
  virtual void SaveCustom(QXmlStreamWriter *writer) const {}
  virtual void PostLoadEvent(SerializedData *data);

  bool LoadInput(QXmlStreamReader *reader, SerializedData *data);
  void SaveInput(QXmlStreamWriter *writer, const QString &id) const;

  bool LoadImmediate(QXmlStreamReader *reader, const QString &input, int element, SerializedData *data);
  void SaveImmediate(QXmlStreamWriter *writer, const QString &input, int element) const;

  void SetFolder(Folder* folder)
  {
    folder_ = folder;
  }

  InputFlag GetInputFlags(const QString& input) const;
  void SetInputFlag(const QString &input, InputFlag f, bool on = true);

  virtual void LoadFinishedEvent(){}
  virtual void ConnectedToPreviewEvent(){}

  static void SetValueAtTime(const NodeInput &input, const rational &time, const value_t::component_t &value, size_t track, MultiUndoCommand *command, bool insert_on_all_tracks_if_no_key);

  /**
   * @brief Find path starting at `from` that outputs to arrive at `to`
   */
  static std::list<NodeInput> FindPath(Node *from, Node *to, int path_index);

  void ArrayResizeInternal(const QString& id, size_t size);

  virtual void AddedToGraphEvent(Project *p){}
  virtual void RemovedFromGraphEvent(Project *p){}

  static QString GetConnectCommandString(const NodeOutput &output, const NodeInput &input);
  static QString GetDisconnectCommandString(const NodeOutput &output, const NodeInput &input);

  static const QString kEnabledInput;

protected:
  void InsertInput(const QString& id, type_t type, size_t channel_count, const value_t& default_value, InputFlag flags, int index);
  void InsertInput(const QString& id, type_t type, const value_t& default_value, InputFlag flags, int index)
  {
    return InsertInput(id, type, 1, default_value, flags, index);
  }

  void PrependInput(const QString& id, type_t type, size_t channel_count, const value_t& default_value, InputFlag flags = kInputFlagNormal)
  {
    InsertInput(id, type, channel_count, default_value, flags, 0);
  }

  void PrependInput(const QString& id, type_t type, const value_t& default_value, InputFlag flags = kInputFlagNormal)
  {
    InsertInput(id, type, 1, default_value, flags, 0);
  }

  void PrependInput(const QString& id, type_t type, InputFlag flags = kInputFlagNormal)
  {
    PrependInput(id, type, value_t(), flags);
  }

  void PrependInput(const QString& id, InputFlag flags = kInputFlagNormal)
  {
    PrependInput(id, TYPE_NONE, value_t(), flags);
  }

  void AddInput(const QString& id, type_t type, size_t channel_count, const value_t& default_value, InputFlag flags = kInputFlagNormal)
  {
    InsertInput(id, type, channel_count, default_value, flags, input_ids_.size());
  }

  void AddInput(const QString& id, type_t type, const value_t& default_value, InputFlag flags = kInputFlagNormal)
  {
    return AddInput(id, type, 1, default_value, flags);
  }

  void AddInput(const QString& id, type_t type, InputFlag flags = kInputFlagNormal)
  {
    AddInput(id, type, value_t(), flags);
  }

  void AddInput(const QString& id, InputFlag flags = kInputFlagNormal)
  {
    AddInput(id, TYPE_NONE, value_t(), flags);
  }

  void RemoveInput(const QString& id);

  void AddOutput(const QString &id);

  void RemoveOutput(const QString &id);

  void SetComboBoxStrings(const QString& id, const QStringList& strings)
  {
    SetInputProperty(id, QStringLiteral("combo_str"), value_t("strl", strings));
  }

  void SendInvalidateCache(const TimeRange &range, const InvalidateCacheOptions &options);

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

  virtual void LinkChangeEvent(){}

  virtual void InputValueChangedEvent(const QString& input, int element);

  virtual void InputConnectedEvent(const QString& input, int element, const NodeOutput &output);

  virtual void InputDisconnectedEvent(const QString& input, int element, const NodeOutput &output);

  virtual void OutputConnectedEvent(const NodeInput& input);

  virtual void OutputDisconnectedEvent(const NodeInput& input);

  virtual void childEvent(QChildEvent *event) override;

  void SetEffectInput(const QString &input)
  {
    effect_input_ = input;
  }

  void SetFlag(Flag f, bool on = true)
  {
    if (on) {
      flags_ |= f;
    } else {
      flags_ &= ~f;
    }
  }

  template<typename T>
  T *AddDraggableGizmo(const QVector<NodeKeyframeTrackReference> &inputs = QVector<NodeKeyframeTrackReference>(), DraggableGizmo::DragValueBehavior behavior = DraggableGizmo::kDeltaFromStart)
  {
    T *gizmo = new T(this);
    gizmo->SetDragValueBehavior(behavior);
    foreach (const NodeKeyframeTrackReference &input, inputs) {
      gizmo->AddInput(input);
    }
    connect(gizmo, &DraggableGizmo::HandleStart, this, &Node::GizmoDragStart);
    connect(gizmo, &DraggableGizmo::HandleMovement, this, &Node::GizmoDragMove);
    return gizmo;
  }

  template<typename T>
  T *AddDraggableGizmo(const QStringList &inputs, DraggableGizmo::DragValueBehavior behavior = DraggableGizmo::kDeltaFromStart)
  {
    QVector<NodeKeyframeTrackReference> refs(inputs.size());
    for (int i=0; i<refs.size(); i++) {
      refs[i] = NodeInput(this, inputs[i]);
    }
    return AddDraggableGizmo<T>(refs, behavior);
  }

  ShaderJob CreateShaderJob(const ValueParams &p, ShaderJob::GetShaderCodeFunction_t f) const
  {
    ShaderJob j = CreateJob<ShaderJob>(p);
    j.set_function(f);
    return j;
  }

  GenerateJob CreateGenerateJob(const ValueParams &p, GenerateJob::GenerateFrameFunction_t f) const
  {
    GenerateJob j = CreateJob<GenerateJob>(p);
    j.set_function(f);
    return j;
  }

  ColorTransformJob CreateColorTransformJob(const ValueParams &p) const
  {
    return CreateJob<ColorTransformJob>(p);
  }

protected slots:
  virtual void GizmoDragStart(const olive::ValueParams &p, double x, double y, const rational &time){}

  virtual void GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers){}

signals:
  /**
   * @brief Signal emitted when SetLabel() is called
   */
  void LabelChanged(const QString& s);

  void ColorChanged();

  void ValueChanged(const NodeInput& input, const TimeRange& range);

  void InputConnected(const NodeOutput &output, const NodeInput& input);

  void InputDisconnected(const NodeOutput &output, const NodeInput& input);

  void OutputConnected(const NodeOutput &output, const NodeInput& input);

  void OutputDisconnected(const NodeOutput &output, const NodeInput& input);

  void InputValueHintChanged(const NodeInput& input);

  void InputPropertyChanged(const QString& input, const QString& key, const value_t& value);

  void LinksChanged();

  void InputArraySizeChanged(const QString& input, int old_size, int new_size);

  void KeyframeAdded(NodeKeyframe* key);

  void KeyframeRemoved(NodeKeyframe* key);

  void KeyframeTimeChanged(NodeKeyframe* key);

  void KeyframeTypeChanged(NodeKeyframe* key);

  void KeyframeValueChanged(NodeKeyframe* key);

  void KeyframeEnableChanged(const NodeInput& input, bool enabled);

  void KeyframeTrackAdded(const QString &input, int element, int track);

  void InputAdded(const QString& id);

  void InputRemoved(const QString& id);

  void OutputAdded(const QString &id);

  void OutputRemoved(const QString &id);

  void InputNameChanged(const QString& id, const QString& name);

  void InputDataTypeChanged(const QString& id, type_t type);

  void AddedToGraph(Project* graph);

  void RemovedFromGraph(Project* graph);

  void NodeAddedToContext(Node *node);

  void NodePositionInContextChanged(Node *node, const QPointF &pos);

  void NodeRemovedFromContext(Node *node);

  void InputFlagsChanged(const QString &input, const InputFlag &flags);

private:
  struct Input {
    type_t type;
    InputFlag flags;
    value_t default_value;
    QHash<QString, value_t> properties;
    QString human_name;
    size_t array_size;
    size_t channel_count;
    std::vector<type_t> id_map;
  };

  NodeInputImmediate* CreateImmediate(const QString& input);

  int GetInternalInputIndex(const QString& input) const
  {
    return input_ids_.indexOf(input);
  }

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

  void SetInputDataTypeInternal(Node::Input *i, const QString &id, type_t type, size_t channel_count);
  void SetInputPropertyInternal(Node::Input *i, const QString &id, const QString& name, const value_t &value);

  void ReportInvalidInput(const char* attempted_action, const QString &id, int element) const;

  static Node *CopyNodeAndDependencyGraphMinusItemsInternal(QMap<Node *, Node *> &created, Node *node, MultiUndoCommand *command);

  /**
   * @brief Immediates aren't deleted, so the actual array size may be larger than ArraySize()
   */
  int GetInternalInputArraySize(const QString& input);

  /**
   * @brief Find nodes of a certain type that this Node takes inputs from
   */
  template<class T>
  static void FindInputNodesConnectedToInputInternal(const NodeInput &input, QVector<T *>& list, int maximum);

  template<class T>
  static void FindInputNodeInternal(const Node* n, QVector<T *>& list, int maximum);

  template<typename T>
  T CreateJob(const ValueParams &p) const
  {
    T job;
    for (const QString &input : inputs()) {
      job.Insert(input, GetInputValue(p, input));
    }
    return job;
  }

  QVector<Node*> GetDependenciesInternal(bool traverse, bool exclusive_only) const;

  void ParameterValueChanged(const QString &input, int element, const TimeRange &range);
  void ParameterValueChanged(const NodeInput& input, const TimeRange &range)
  {
    ParameterValueChanged(input.input(), input.element(), range);
  }

  /**
   * @brief Intelligently determine how what time range is affected by a keyframe
   */
  TimeRange GetRangeAffectedByKeyframe(NodeKeyframe *key) const;

  /**
   * @brief Gets a time range between the previous and next keyframes of index
   */
  TimeRange GetRangeAroundIndex(const QString& input, int index, int track, int element) const;

  void ClearElement(const QString &input, int index);

  type_t ResolveSpecialType(type_t type, size_t &channel_count, QString &subtype, std::vector<type_t> &id_map);

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
  QVector<Output> outputs_;

  QMap<QString, NodeInputImmediate*> standard_immediates_;

  QMap<QString, QVector<NodeInputImmediate*> > array_immediates_;

  Connections input_connections_;

  Connections output_connections_;

  Folder* folder_;

  QMap<InputElementPair, ValueHint> value_hints_;

  PositionMap context_positions_;

  uint64_t flags_;

  QVector<NodeGizmo*> gizmos_;

  QString effect_input_;

  FrameHashCache *video_cache_;
  ThumbnailCache *thumbnail_cache_;

  AudioPlaybackCache *audio_cache_;
  AudioWaveformCache *waveform_cache_;

  bool caches_enabled_;

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
void Node::FindInputNodesConnectedToInputInternal(const NodeInput &input, QVector<T *> &list, int maximum)
{
  Node* edge = input.GetConnectedOutput();
  if (!edge) {
    return;
  }

  T* cast_test = dynamic_cast<T*>(edge);

  if (cast_test) {
    list.append(cast_test);
    if (maximum != 0 && list.size() == maximum) {
      return;
    }
  }

  FindInputNodeInternal<T>(edge, list, maximum);
}

template<class T>
QVector<T *> Node::FindInputNodesConnectedToInput(const NodeInput &input, int maximum)
{
  QVector<T *> list;

  FindInputNodesConnectedToInputInternal<T>(input, list, maximum);

  return list;
}

template<class T>
void Node::FindInputNodeInternal(const Node* n, QVector<T *> &list, int maximum)
{
  for (auto it=n->input_connections_.cbegin(); it!=n->input_connections_.cend(); it++) {
    FindInputNodesConnectedToInputInternal(it->second, list, maximum);
  }
}

template<class T>
QVector<T *> Node::FindInputNodes(int maximum) const
{
  QVector<T *> list;

  FindInputNodeInternal<T>(this, list, maximum);

  return list;
}

}

Q_DECLARE_METATYPE(olive::Node::ValueHint)

#endif // NODE_H
