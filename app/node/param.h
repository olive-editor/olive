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

#ifndef NODEPARAM_H
#define NODEPARAM_H

#include <QMutex>
#include <QObject>
#include <QVariant>
#include <QVector>

#include "common/rational.h"
#include "node/edge.h"

class Node;

/**
 * @brief A base parameter of a Node
 *
 * The main data points of a Node. NodeParams are added to Nodes so that Node::Process() can use data acquired either
 * directly as a value set by the user, or through the output of another NodeParam.
 *
 * This is an abstract base class. In most cases you'll want NodeInput or NodeOutput.
 */
class NodeParam : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief The type of parameter this is
   */
  enum Type {
    kInput,
    kOutput
  };

  /**
   * @brief The types of data that can be passed between Nodes
   */
  enum DataType {
    kNone,

    /// Resolves to `int`
    kInt,

    /// Resolves to `double`
    kFloat,

    /// Resolves to TBA
    kColor,

    /// Resolves to `QString`
    kString,

    /// Resolves to `bool`
    kBoolean,

    /// Resolves to TBA
    kFont,

    /// Resolves to `QString` filename
    kFile,

    /// Resolves to `RenderTexturePtr`
    kTexture,

    /// Resolves to `QMatrix4x4`
    kMatrix,

    /// Resolves to `Block*`
    kBlock,

    /// Resolves to `QList<Block*>`
    kBlockList,

    /// Resolves to `Footage*`
    kFootage,

    /// Resolves to `TrackOutput*`
    kTrack,

    /// Resolves to `rational`
    kRational,

    /// Resolves to `QVector2D`
    kVec2,

    /// Resolves to `QVector3D`
    kVec3,

    /// Resolves to `QVector4D`
    kVec4,

    /// Resolves to `QByteArray`
    kSamples,

    kAny
  };

  /**
   * @brief NodeParam Constructor
   */
  NodeParam(const QString& id);

  /**
   * @brief Return ID of this parameter
   */
  const QString id();

  /**
   * @brief The type of node paramter this is
   *
   * This must be set in subclasses, but most of the time you should probably subclass from NodeInput and NodeOutput
   * anyway.
   */
  virtual Type type() = 0;

  /**
   * @brief Name of this parameter to be shown to the user
   */
  virtual QString name();
  void set_name(const QString& name);

  /**
   * @brief Node parent object
   *
   * Nodes and NodeParams use the QObject parent-child system. This function is a convenience function for
   * static_cast<Node*>(QObject::parent())
   */
  Node* parent();

  /**
   * @brief Return the row index of this parameter in the parent node (primarily used for UI drawing functions)
   */
  int index();

  /**
   * @brief Returns whether anything is connected to this parameter or not
   */
  bool IsConnected();

  /**
   * @brief Return a list of edges (aka connections to other nodes)
   *
   * This list can't be modified directly. Use ConnectEdge() and DisconnectEdge() instead for that.
   */
  const QVector<NodeEdgePtr>& edges();

  /**
   * @brief Disconnect any edges connecting this parameter to other parameters
   */
  void DisconnectAll();

  /**
   * @brief Connect an output parameter to an input parameter
   *
   * This function makes no attempt to check whether the two NodeParams have compatible data types. This should be done
   * beforehand or behavior is undefined.
   *
   * If the input already has an edge connected and can't accept multiple edges, that edge is disconnected before an
   * attempt at a new connection is made. This function returns the new NodeEdge created by this connection.
   *
   * If the input *can* accept multiple edges but is already connected to this output, no new connection is made (since
   * the connection already exists). In this situation, nullptr is returned.
   *
   * This function emits EdgeAdded().
   */
  static NodeEdgePtr ConnectEdge(NodeOutput *output, NodeInput *input);

  /**
   * @brief Disconnect an edge
   *
   * This function emits EdgeRemoved(NodeEdgePtr edge).
   *
   * @param edge
   *
   * Edge to disconnect.
   */
  static void DisconnectEdge(NodeEdgePtr edge);

  /**
   * @brief Disconnect an edge
   *
   * Sometimes this function is preferable if you don't know what the edge object is (or with undo commands where the
   * edge object may change despite the connection being between the same parameters).
   *
   * This function emits EdgeRemoved(NodeEdgePtr edge).
   *
   * @param edge
   *
   * Edge to disconnect.
   */
  static void DisconnectEdge(NodeOutput* output, NodeInput* input);

  /**
   * @brief If an input has an edge and can't take multiple, this function disconnects them and returns the edge object
   *
   * This is used just before a connection is about to be made. If an input is already connected to an output, but
   * can't take multiple inputs, that connection will need to be removed before the new connection can be made.
   * This function check if it's necessary to remove the edge from an input before connecting a new edge, and removes
   * and returns it if so.
   *
   * If the input does NOT have anything connected, or it does but the input CAN accept multiple connections, nothing
   * is disconnected and nullptr is returned.
   */
  static NodeEdgePtr DisconnectForNewOutput(NodeInput* input);

  /**
   * @brief Get a human-readable translated name for a certain data type
   */
  static QString GetDefaultDataTypeName(const DataType &type);

  /**
   * @brief Convert a value from a NodeParam into bytes
   */
  static QByteArray ValueToBytes(const DataType &type, const QVariant& value);

  /**
   * @brief Clear the cached value
   */
  void ClearCachedValue();

  /**
   * @brief Retrieve the last time this parameter had a value requested from
   */
  const rational& LastRequestedIn();

  bool ValueCachingEnabled();
  void SetValueCachingEnabled(bool enabled);

signals:
  /**
   * @brief Signal emitted when an edge is added to this parameter
   *
   * See ConnectEdge() for usage. Only one of the two parameters needs to emit this signal when a connection is made,
   * because otherwise two of exactly the same signal will be emitted.
   */
  void EdgeAdded(NodeEdgePtr edge);

  /**
   * @brief Signal emitted when an edge is removed from this parameter
   *
   * See DisconnectEdge() for usage. Only one of the two parameters needs to emit this signal when a connection is
   * removed, because otherwise two of exactly the same signal will be emitted.
   */
  void EdgeRemoved(NodeEdgePtr edge);

protected:
  /**
   * @brief Internal list of edges
   */
  QVector<NodeEdgePtr> edges_;

  /**
   * @brief Currently cached value
   */
  QVariant value_;

  /**
   * @brief Last timecodes that a value was requested with
   */
  rational in_;
  rational out_;

  /**
   * @brief Internal value for whether value caching is enabled
   */
  bool value_caching_;

  /**
   * @brief Internal name string
   */
  QString name_;

private:
  /**
   * @brief Internal function for returning a value in the form of bytes
   */
  template<typename T>
  static QByteArray ValueToBytesInternal(const QVariant& v);

  /**
   * @brief Internal ID string
   */
  QString id_;

};

#endif // NODEPARAM_H
