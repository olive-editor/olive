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
  QString name();
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
   * @brief Determine whether two DataTypes are compatible and therefore whether two NodeParams can be connected
   *
   * Obviously a data type is compatible with itself, but sometimes fundamentally separate data types may still be
   * allowed to connect (e.g. an integer output to a float input). This static function should be used to determine
   * whether a data type is compatible with another.
   */
  static bool AreDataTypesCompatible(const DataType& output_type, const DataType& input_type);

  /**
   * @brief Overload of AreDataTypesCompatible(const DataType& output_type, const DataType& input_type)
   *
   * Use this for a list of input data types (which NodeInput uses as it's possible for it to accept multiple types).
   */
  static bool AreDataTypesCompatible(const DataType& output_type, const QList<DataType>& input_types);

  /**
   * @brief Overload of AreDataTypesCompatible(const DataType& output_type, const DataType& input_type)
   *
   * Convenience function for two NodeParams. Determines which is the input/output and determines whether their types
   * are compatible.
   */
  static bool AreDataTypesCompatible(NodeParam* a, NodeParam* b);

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
   * @brief Clear the cached value
   */
  void ClearCachedValue();

  /**
   * @brief Retrieve the last time this parameter had a value requested from
   */
  const rational& LastRequestedTime();

  virtual DataType data_type() = 0;

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
   * @brief Last timecode that a value was requested with
   */
  rational time_;

private:
  /**
   * @brief Internal name string
   */
  QString name_;

  /**
   * @brief Internal ID string
   */
  QString id_;

};

#endif // NODEPARAM_H
