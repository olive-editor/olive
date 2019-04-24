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

#ifndef EFFECTUI_H
#define EFFECTUI_H

#include "collapsiblewidget.h"
#include "nodes/node.h"
#include "ui/nodeui.h"
#include "ui/keyframenavigator.h"

/**
 * @brief The EffectUI class
 *
 * EffectUI is a complete QWidget-based representation of an Effect that can be added to any Qt layout. It overrides
 * CollapsibleWidget (meaning the Effect can be collapsed to just a titlebar to save space). The titlebar is
 * automatically set to the Effect's name and the contents are composed of a grid layout (QGridLayout) corresponding
 * to the Effect's EffectRow and EffectField children.
 *
 * Many EffectUIs can be created from a single Effect, and many Effects can be attached to a single EffectUI (provided
 * the Effects are all the same type). Neither gains ownership of each other and deleting an EffectUI without any other
 * work is perfectly safe (deleting an Effect with an open EffectUI however, is not).
 */
class EffectUI : public CollapsibleWidget {
  Q_OBJECT
public:
  /**
   * @brief EffectUI Constructor
   *
   * Creates a QWidget-based UI representation of an Effect.
   *
   * @param e
   *
   * The Effect to make a UI of. It must be a valid object.
   */
  EffectUI(Node* e);

  /**
   * @brief Attach additional effects to this UI
   *
   * Olive allows users to modify several effects (of the same type) with one UI representation. To do this, you can
   * add any amount of extra Effect objects using this function and the UI will attach all of its UI functions to that
   * Effect as well without creating any new QWidgets.
   *
   * @param e
   *
   * The Effect to add to this UI object.
   */
  void AddAdditionalEffect(Node* e);

  /**
   * @brief Get the primary Effect that this UI object was created for
   *
   * @return
   *
   * The Effect object passed to the constrcutor whe creating this EffectUI.
   */
  Node* GetEffect();

  /**
   * @brief Get the Y position of a given row
   *
   * Retrieve the on-screen Y position of the attached Effect's EffectRow at a given index. This is primarily used for
   * displaying UI elements that align with the the row's on-screen widgets (e.g. keyframes in the EffectControls
   * panel).
   *
   * The Y value provided is specifically the center point of the row's name label. It gets mapped to a provided
   * QWidget object so it can be used locally by that QWidget without further modification.
   *
   * @param row
   *
   * The index of the EffectRow to retrieve the Y position of.
   *
   * @param mapToWidget
   *
   * The widget to map the Y value to.
   *
   * @return
   *
   * The row's Y position.
   */
  int GetRowY(int row, QWidget *mapToWidget = nullptr);

  /**
   * @brief Update widgets with the current Effect's values.
   *
   * When the Timeline playhead moves, the current values in the Effect might change if its fields are keyframed.
   * In order to visually update these values on the UI, this function should be called. It will loop through all
   * fields of all attached effects and update them to the value at the current Timeline playhead.
   *
   * Currently this function is called by update_ui() which is also responsible for updating other parts of the UI
   * like the Timeline and Viewer so they all get updated together.
   */
  void UpdateFromEffect();

  /**
   * @brief Check if a given Clip has an Effect referenced by this EffectUI
   *
   * Olive allows users to modify several effects (of the same type) with one UI representation. The behavior is if
   * multiple clips are selected that have effects of the same type, all those Effects can be modified by the same
   * EffectUI object. However this behavior is undesirable if a single Clip has more than one of the same type of Effect
   * (e.g. two or more blurs). In this scenario, the user will most likely expect two separate UI objects for each of
   * these effects individually, rather than consolidating them into one UI object.
   *
   * To address this, EffectControls will check this function to determine if this EffectUI already references
   * an Effect of this type from this Clip. If it does, it's assumed a new EffectUI should be made rather than
   * consolidating that Effect into the same EffectUI.
   *
   * @param c
   *
   * The Clip to determine whether an Effect of this type is already referenced by this EffectUI.
   *
   * @return
   *
   * True is an Effect from this Clip is already attached to this EffectUI.
   */
  bool IsAttachedToClip(Clip* c);

protected:

signals:
  /**
   * @brief Cut signal
   *
   * Emitted when the user selects Cut from the right-click context menu.
   */
  void CutRequested();

  /**
   * @brief Copy signal
   *
   * Emitted when the user selects Copy from the right-click context menu.
   */
  void CopyRequested();
private:
  /**
   * @brief Retrieve the QWidget corresponding a specific EffectField
   *
   * Convenience function equivalent to widgets_.at(row).at(field).
   *
   * @param row
   *
   * EffectRow index to retrieve field QWidget from
   *
   * @param field
   *
   * EffectField index to retrieve QWidget from
   *
   * @return
   *
   * The QWidget at this row and field index.
   */
  QWidget* Widget(int row, int field);

  /**
   * @brief Internal reference to the Effect this object was constructed around.
   */
  Node* effect_;

  /**
   * @brief Internal array of additional Effect objects attached to this UI.
   */
  QVector<Node*> additional_effects_;

  /**
   * @brief Layout for UI widgets
   */
  QGridLayout* layout_;

  /**
   * @brief Grid array of QWidgets corresponding to the Effect's rows and fields
   */
  QVector< QVector<QWidget*> > widgets_;

  /**
   * @brief Array of QLabel objects corresponding to each row's name().
   */
  QVector<QLabel*> labels_;

  /**
   * @brief Array of KeyframeNavigator objects corresponding to each row.
   */
  QVector<KeyframeNavigator*> keyframe_navigators_;

  /**
   * @brief Attach a KeyframeNavigator object to an EffectRow.
   *
   * Internal function for connecting a KeyframeNavigator UI object to an EffectRow.
   *
   * @param row
   *
   * The EffectRow object.
   *
   * @param nav
   *
   * The KeyframeNavigator object.
   */
  void AttachKeyframeNavigationToRow(EffectRow* row, KeyframeNavigator* nav);
private slots:
  /**
   * @brief Slot for titlebar's right-click signal to show a context menu for extra Effect functions.
   */
  void show_context_menu(const QPoint&);
};

#endif // EFFECTUI_H
