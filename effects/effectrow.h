/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#ifndef EFFECTROW_H
#define EFFECTROW_H

#include <QObject>
#include <QVector>

class Node;
class QGridLayout;
class EffectField;
class QLabel;
class QPushButton;
class ComboAction;
class QHBoxLayout;
class KeyframeNavigator;
class ClickableLabel;

#include "effectfields.h"
#include "nodes/nodeedge.h"

/**
 * @brief The EffectRow class
 *
 * Primarily a way of grouping EffectField objects together. As UI objects, Effects are largely formatted as a table
 * and you can think of EffectRows as the rows of the table and EffectFields as the columns.
 *
 * Within Olive, keyframing is enabled on the EffectRow (rather than individual EffectFields) so all attached fields
 * will be keyframed together.
 *
 * Unlike EffectField, there's no reason to derive from EffectRow as it's simply a container of fields and a few
 * keyframe functions.
 */
class EffectRow : public QObject {
  Q_OBJECT
public:

  /**
   * @brief EffectRow Constructor
   *
   * @param parent
   *
   * Every EffectRow object must be attached to a valid Effect object. The Effect object takes ownership of the
   * EffectRow and automatically frees it through the QObject parent/child system. EffectRows are never intended to
   * change parents throughout their lifetimes.
   *
   * @param id
   *
   * Field ID. Must be non-empty. Must also be unique within this Effect. Used for saving/loading values into project
   * files so that if ordering of fields are changed, or fields are added/removed from Effects later in development,
   * saved values in project files will still link with the correct field. Also used as the uniform variable name
   * in GLSL shaders.
   *
   * @param name
   *
   * Row name. This is not used as an internal identifier, it's just for the user interface, so it can be translated
   * with no issue.
   *
   * @param savable
   *
   * Whether the fields in this row should be saved to the project file. This is true by default. If the row contains
   * non-value UI widgets, setting this to false is recommended.
   *
   * @param keyframable
   *
   * Whether keyframing can be enabled on this row or not. This is true by default. Some values you may want to prevent
   * the user from keyframing (e.g. the filename of a VST plugin), which can be done by setting this to false.
   */
  EffectRow(Node* parent,
            const QString& id,
            const QString& name,
            bool savable = true,
            bool keyframable = true);

  /**
   * @brief Retrieve the EffectField at this index. Must be less than FieldCount().
   *
   * @param i
   *
   * Index to retrieve the EffectField at.
   *
   * @return
   *
   * EffectField at the provided index.
   */
  EffectField* Field(int i);

  /**
   * @brief Number of fields currently contained in this row.
   *
   * @return The number of fields currently contained in this row as an integer. Any field index (retrieved with
   * Field()) is guaranteed to be valid >= 0 and < FieldCount().
   */
  int FieldCount();

  /**
   * @brief Set a keyframe at the current playhead on all fields contained within this row
   *
   * @param ca
   *
   * The ComboAction to add this action to. This may not be nullptr.
   */
  void SetKeyframeOnAllFields(ComboAction *ca);

  /**
   * @brief Get parent Effect
   *
   * Equivalent to `static_cast<Effect*>(parent())`.
   *
   * @return The parent Effect object that this row is attached to.
   */
  Node* GetParentEffect();

  /**
   * @brief Return the row's name
   *
   * @return The name specified in the constructor as a QString
   */
  const QString& name();

  /**
   * @brief Get the unique identifier of this field set in the constructor
   *
   * Mostly used for saving/loading or interacting with GLSL-based shader effects (see EffectField() for more details).
   *
   * @return
   */
  const QString& id();

  /**
   * @brief Get whether this row is keyframing or not
   *
   * @return True if this row is keyframing.
   */
  bool IsKeyframing();

  /**
   * @brief Set whether this row is keyframing or not.
   *
   * It's not recommended to use this function for any user-initiated keyframe setting change as it doesn't create any
   * undoable actions. Use SetKeyframingEnabled() instead for a user-friendly variant.
   */
  void SetKeyframingInternal(bool);

  /**
   * @brief Get whether this row should be saved into a project file or not
   *
   * @return True if this row should be saved. This value is set in the constructor.
   */
  bool IsSavable();

  /**
   * @brief Get whether this row can be keyframed or not.
   *
   * @return True if this row can be keyframed. This value is set in the constructor.
   */
  bool IsKeyframable();

  /**
   * @brief Get value at a given timecode
   *
   * Functions as a wrapper for EffectField::GetValueAt().
   *
   * The default function is to return the result of the first EffectField on this EffectRow which should be sufficient
   * for most input types. If this EffectRow has more or less than one EffectField, you must override this function in a
   * derived class to provide the correct field <-> row coordination or else it will trigger an abort.
   *
   * @param timecode
   *
   * Timecode to get the value at
   *
   * @return
   *
   * The value of the first EffectField at the given timecode
   */
  virtual QVariant GetValueAt(double timecode);

  /**
   * @brief SetValueAt
   *
   * Functions as a wrapper for EffectField::SetValueAt().
   *
   * The default function is to call the function of the first EffectField on this EffectRow which should be sufficient
   * for most input types. If this EffectRow has more or less than one EffectField, you must override this function in a
   * derived class to provide the correct field <-> row coordination or else it will trigger an abort.
   *
   * @param timecode
   *
   * Timecode to set the value at
   *
   * @param value
   *
   * Value to set at this timecode
   */
  virtual void SetValueAt(double timecode, const QVariant& value);

  /**
   * @brief Sets the enabled state on all EffectField objects on this row to enabled
   */
  void SetEnabled(bool enabled);

  /**
   * @brief Check if nodes can be connected to this as an input.
   *
   * Connecting is enabled by adding an accepted node input using AddNodeInput().
   *
   * @return
   *
   * TRUE if nodes can be connected as an input.
   */
  bool IsNodeInput();

  /**
   * @brief Check if nodes can be connected to this as an output
   *
   * Connecting is enabled by setting an output data type in SetOutputDataType().
   *
   * @return
   *
   * TRUE if nodes can be connected as an output
   */
  bool IsNodeOutput();

  /**
   * @brief Set output data type
   *
   * Set the type of data this row outputs to type
   */
  void SetOutputDataType(olive::nodes::DataType type);

  /**
   * @brief Check if this row can accept the data type specified in type
   *
   * @return
   *
   * TRUE if this row can accept this data type. If this row is not an input, this function will always return FALSE.
   */
  bool CanAcceptDataType(olive::nodes::DataType type);

  /**
   * @brief Get this row's output data type
   *
   * @return
   *
   * If this is not an output, this will always return olive::nodes::kInvalid
   */
  olive::nodes::DataType OutputDataType();

  /**
   * @brief Adds a node data type that can be accepted by this input
   *
   * Allows this input to take a node connection from a data type specified by type. An input can take several data
   * types.
   *
   * @param type
   *
   * The data type to add
   */
  void AddAcceptedNodeInput(olive::nodes::DataType type);

  /**
   * @brief Connect two node sockets together
   *
   * For outputs, the maximum amount of edges is unlimited and this will continually add to a list of edges. For inputs,
   * there can only be one edge and adding a second will destroy the first.
   *
   * @param edge
   *
   * The edge to add (output) or replace the current edge (input).
   */
  static void ConnectEdge(EffectRow* output, EffectRow* input);

  /**
   * @brief Disconnect an edge
   *
   * Disconnects two nodes and destroys the edge object connecting them together
   *
   * @param edge
   *
   * Edge to be removed
   */
  static void DisconnectEdge(NodeEdgePtr edge);

  /**
   * @brief Get a list of references to all edges currently connected to this row
   */
  QVector<NodeEdge *> edges();

protected:
  /**
   * @brief Add a field to this row
   *
   * Ownership of the EffectField is transferred to this row and the row will free its memory. In the Effect's UI, this
   * will add the field to an additional column.
   *
   * @param Field
   *
   * The field to add to this row.
   */
  void AddField(EffectField* Field);

public slots:
  /**
   * @brief Go to previous keyframe
   *
   * Gets the closest keyframe prior to the current playhead and seeks to it.
   *
   * Attach to KeyframeNavigator::goto_previous_key() signal.
   */
  void GoToPreviousKeyframe();

  /**
   * @brief Toggle a keyframe at this point in time
   *
   * Either deletes (if any child EffectFields have any keyframes here) or creates (if none do) a keyframe on all
   * EffectField children at the current time.
   *
   * Attach to KeyframeNavigator::toggle_key() signal.
   */
  void ToggleKeyframe();

  /**
   * @brief Go to next keyframe
   *
   * Gets the closest keyframe after the current playhead and seeks to it.
   *
   * Attach to KeyframeNavigator::goto_next_key() signal.
   */
  void GoToNextKeyframe();

  /**
   * @brief Slot for whenever this EffectRow is focused.
   *
   * Connect UI objects gaining focus to this slot. Automatically updates the Graph Editor to attach to this row.
   */
  void FocusRow();
signals:

  /**
   * @brief Keyframing setting changed signal
   *
   * Emitted whenever keyframing is enabled or disabled.
   *
   * @param
   *
   * True if keyframing was enabled, false if keyframing was disabled.
   */
  void KeyframingSetChanged(bool);

  /**
   * @brief Changed signal
   *
   * Wrapper for EffectField::Changed().
   */
  void Changed();

  /**
   * @brief Clicked signal
   *
   * Wrapper for EffectField::Clicked().
   */
  void Clicked();

  /**
   * @brief Edges changed signal
   *
   * Signal emitted any time an edge is connected or disconnected from this row
   */
  void EdgesChanged();

private slots:
  /**
   * @brief Set keyframing enabled state
   *
   * A user-friendly function for enabling or disabling keyframes on this row. Preferred to SetKeyframingInternal()
   * for any user-initiated change. Automatically creates an undoable action so users can undo the enabling/disabling.
   * Also confirms with the user when disabling keyframing whether they wish to continue and remove all the current
   * keyframes.
   *
   * Attach to KeyframeNavigator::keyframe_enabled_changed() signal.
   */
  void SetKeyframingEnabled(bool);
private:

  /**
   * @brief Internal unique identifier for this field set in the constructor. Access with id().
   */
  QString id_;

  /**
   * @brief Internal variable for the row's name
   *
   * Set in the constructor, retrieved with name().
   */
  QString name_;

  /**
   * @brief Internal variable for whether this row can be keyframed.
   *
   * Set in the constructor, retrieved with IsKeyframable().
   */
  bool keyframable_;

  /**
   * @brief Internal variable for whether this row is currently keyframing.
   *
   * Set by SetKeyframingInternal() and retrieved with IsKeyframing().
   */
  bool keyframing_;

  /**
   * @brief Internal variable for whether this row should be saved.
   *
   * Set in the constructor, retrieved with IsSavable().
   */
  bool savable_;

  /**
   * @brief Internal array of EffectField objects.
   *
   * It is not necessary to delete the elements in this array as they're already children of this QObject, so they'll
   * get freed automatically.
   */
  QVector<EffectField*> fields_;

  /**
   * @brief Internal array of accepted node data types.
   *
   * Is mutally-exclusive with accepted_outputs_, i.e. you cannot have values added to this and also a value set in
   * accepted_outputs_.
   */
  QVector<olive::nodes::DataType> accepted_inputs_;

  /**
   * @brief Internal value for what kind of data this row outputs
   *
   * Is mutally-exclusive with accepted_inputs_, i.e. you cannot have values added to it and also a value set in
   * this.
   */
  olive::nodes::DataType output_type_;

  /**
   * @brief Internal array of node edges. Access with AddEdge() and RemoveEdge().
   */
  QVector<NodeEdgePtr> node_edges_;
};

#endif // EFFECTROW_H
