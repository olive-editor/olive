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
#include <QMutex>
#include <QObject>

#include "common/rational.h"
#include "node/dependency.h"
#include "node/input.h"
#include "node/output.h"

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
  Node();

  /**
   * @brief Return the name of the node
   *
   * This is the node's name shown to the user. This must be overridden by subclasses, and preferably run through the
   * translator.
   */
  virtual QString Name() = 0;

  /**
   * @brief Return the unique identifier of the node
   *
   * This is used in save files and any other times a specific node must be picked out at runtime. This must be an ID
   * completely unique to this node, and preferably in bundle identifier format (e.g. "org.company.Name"). This string
   * should NOT be translated.
   */
  virtual QString id() = 0;

  /**
   * @brief Return the category this node is in (optional for subclassing, but recommended)
   *
   * In any organized node menus, show the node in this category. If this node should be in a subfolder of a subfolder,
   * use a "/" to separate categories (e.g. "Distort/Noise"). The string should not start with a "/" as this will be
   * interpreted as an empty string category. This value should be run through a translator as its largely user
   * oriented.
   */
  virtual QString Category();

  /**
   * @brief Return a description of this node's purpose (optional for subclassing, but recommended)
   *
   * A short (1-2 sentence) description of what this node should do to help the user understand its purpose. This should
   * be run through a translator.
   */
  virtual QString Description();

  /**
   * @brief Signals the Node that it won't be used for a while and can deallocate some memory
   */
  virtual void Release();

  /**
   * @brief Function called to retranslate parameter names (should be overridden in derivatives)
   */
  virtual void Retranslate();

  /**
   * @brief Return the parameter at a given index
   */
  NodeParam* ParamAt(int index);

  /**
   * @brief Return a list of NodeParams
   */
  QList<NodeParam*> parameters();

  /**
   * @brief Return the current number of parameters
   */
  int ParameterCount();

  /**
   * @brief Return the index of a parameter
   * @return Parameter index or -1 if this parameter is not part of this Node
   */
  int IndexOfParameter(NodeParam* param);

  /**
   * @brief Return a list of all Nodes that this Node's inputs are connected to (does not include this Node)
   */
  QList<Node*> GetDependencies();

  /**
   * @brief Returns a list of Nodes that this Node is dependent on, provided no other Nodes are dependent on them
   * outside of this hierarchy.
   *
   * Similar to GetDependencies(), but excludes any Nodes that are used outside the dependency graph of this Node.
   */
  QList<Node*> GetExclusiveDependencies();

  /**
   * @brief Retrieve immediate dependencies (only nodes that are directly connected to the inputs of this one)
   */
  QList<Node*> GetImmediateDependencies();

  /**
   * @brief Wrapper for Process()
   *
   * It's recommended to call this directly over Value(), yet in derivatives of Node, override Value().
   */
  QVariant Run(NodeOutput* output, const rational& time);

  /**
   * @brief For nodes that have different dependencies at different times, this function can be used for that purpose
   *
   * Only retrieves immmediate dependencies, meaning only nodes that are directly
   */
  virtual QList<NodeDependency> RunDependencies(NodeOutput* output, const rational& time);

  /**
   * @brief Returns whether this Node outputs data to the Node `n` in any way
   */
  bool OutputsTo(Node* n);

  /**
   * @brief Add's unique information about this Node at the given time to a QCryptographicHash
   */
  virtual void Hash(QCryptographicHash* hash, NodeOutput *from, const rational& time);

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
  virtual void InvalidateCache(const rational& start_range, const rational& end_range, NodeInput* from = nullptr);

  /**
   * @brief Lock mutex (for thread safety)
   */
  void Lock();

  /**
   * @brief Unock mutex (for thread safety)
   */
  void Unlock();

  /**
   * @brief Copies inputs from from Node to another including connections
   *
   * Nodes must be of the same types (i.e. have the same ID)
   */
  static void CopyInputs(Node* source, Node* destination);

protected:
  /**
   * @brief Add a parameter to this node
   *
   * The Node takes ownership of this parameter.
   *
   * This can be either an output or an input at any time. Parameters will always appear in the order they're added.
   */
  void AddParameter(NodeParam* param);

  /**
   * @brief Deletes the parameter from this Node
   *
   * The NodeParam object is destroyed in the process.
   */
  void RemoveParameter(NodeParam* param);

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
  virtual QVariant Value(NodeOutput* output, const rational& time) = 0;

  /**
   * @brief Retrieve the last timecode Process() was called with
   */
  rational LastProcessedTime();

  /**
   * @brief Retrieve the last parameter Process() was called from
   */
  NodeOutput* LastProcessedOutput();

  void ClearCachedValuesInParameters(const rational& start_range, const rational& end_range);

  void SendInvalidateCache(const rational& start_range, const rational& end_range);

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

private:
  /**
   * @brief Return whether a parameter with ID `id` has already been added to this Node
   */
  bool HasParamWithID(const QString& id);

  /**
   * @brief The last timecode Process() was called with
   */
  rational last_processed_time_;

  /**
   * @brief The last parameter Process() was called from
   */
  NodeOutput* last_processed_parameter_;

  /**
   * @brief Used for thread safety
   */
  QMutex lock_;

private slots:
  void InputChanged(rational start, rational end);

  void InputConnectionChanged(NodeEdgePtr edge);

};

template<class T>
T* Node::ValueToPtr(const QVariant &ptr)
{
  return reinterpret_cast<T*>(ptr.value<quintptr>());
}

#endif // NODE_H
