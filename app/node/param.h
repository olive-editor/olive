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

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QXmlStreamWriter>

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
    kNone = 0x0,

    /**
     ****************************** SPECIFIC IDENTIFIERS ******************************
     */

    /**
     * Integer type
     *
     * Resolves to `int` (may resolve to `long` in the future).
     */
    kInt = 0x1,

    /**
     * Decimal (floating-point) type
     *
     * Resolves to `double`.
     */
    kFloat = 0x2,

    /**
     * Decimal (rational) type
     *
     * Resolves to `double`.
     */
    kRational = 0x4,

    /**
     * Boolean type
     *
     * Resolves to `bool`.
     */
    kBoolean = 0x8,

    /**
     * Floating-point type
     *
     * Resolves to `QRgba64`.
     *
     * Colors passed around the nodes should always be in reference space and preferably use
     */
    kColor = 0x10,

    /**
     * Matrix type
     *
     * Resolves to `QMatrix4x4`.
     */
    kMatrix = 0x20,

    /**
     * Text type
     *
     * Resolves to `QString`.
     */
    kText = 0x40,

    /**
     * Font type
     *
     * Resolves to `QFont`.
     */
    kFont = 0x80,

    /**
     * File type
     *
     * Resolves to a `QString` containing an absolute file path.
     */
    kFile = 0x100,

    /**
     * Image buffer type
     *
     * True value type depends on the render engine used.
     */
    kTexture = 0x200,

    /**
     * Audio samples type
     *
     * Resolves to `QVector4D`.
     */
    kSamples = 0x400,

    /**
     * Footage stream identifier type
     *
     * Resolves to `StreamPtr`.
     */
    kFootage = 0x800,

    /**
     * Two-dimensional vector (XY) type
     *
     * Resolves to `QVector2D`.
     */
    kVec2 = 0x1000,

    /**
     * Three-dimensional vector (XYZ) type
     *
     * Resolves to `QVector3D`.
     */
    kVec3 = 0x2000,

    /**
     * Four-dimensional vector (XYZW) type
     *
     * Resolves to `QVector4D`.
     */
    kVec4 = 0x4000,

    /**
     ****************************** BROAD IDENTIFIERS ******************************
     */

    /**
     * Identifier for type that contains a decimal number
     *
     * Includes kFloat and kRational.
     */
    kDecimal = 0x6,

    /**
     * Identifier for type that contains a number of any kind (whole or decimal)
     *
     * Includes kInt, kFloat, and kRational.
     */
    kNumber = 0x7,

    /**
     * Identifier for type that contains a text string of any kind.
     *
     * Includes kText and kFile.
     */
    kString = 0x140,

    /**
     * Identifier for type that contains a either an image or audio buffer
     *
     * Includes kTexture and kSamples.
     */
    kBuffer = 0x600,

    /**
     * Identifier for type that contains a vector (two- to four-dimensional)
     *
     * Includes kVec2, kVec3, kVec4, and kColor.
     */
    kVector = 0x7010,

    /**
     * Identifier for any type
     *
     * Matches with all types except for kNone
     */
    kAny = 0xFFFFFFFF
  };

  /**
   * @brief NodeParam Constructor
   */
  NodeParam(const QString& id);

  virtual ~NodeParam() override;

  struct SerializedConnection {
    NodeInput* input;
    quintptr output;
  };

  struct FootageConnection {
    NodeInput* input;
    quintptr footage;
  };

  /**
   * @brief Load function
   */
  virtual void Load(QXmlStreamReader* reader, QHash<quintptr, NodeOutput*>& param_ptrs, QList<SerializedConnection> &input_connections, QList<FootageConnection>& footage_connections, const QAtomicInt* cancelled) = 0;

  /**
   * @brief Save function
   */
  virtual void Save(QXmlStreamWriter* writer) const = 0;

  /**
   * @brief Return ID of this parameter
   */
  const QString id() const;

  /**
   * @brief The type of node parameter this is
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
  Node* parentNode() const;

  /**
   * @brief Return the row index of this parameter in the parent node (primarily used for UI drawing functions)
   */
  int index();

  /**
   * @brief Returns whether anything is connected to this parameter or not
   */
  bool IsConnected() const;

  bool IsConnectable() const;
  void SetConnectable(bool connectable);

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
   * @brief Convert a string to a data type
   */
  static DataType StringToDataType(const QString& s);

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

  /**
   * @brief Internal connectable value
   */
  bool connectable_;

};

#endif // NODEPARAM_H
