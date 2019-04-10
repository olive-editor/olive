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

#ifndef COMBOFIELD_H
#define COMBOFIELD_H

#include "../effectfield.h"

/**
 * @brief The ComboFieldItem struct
 *
 * An internal string+value pair used to represent a combobox item. name is used for the UI
 * representation of the choices and the data is what can be retrieved by code.
 *
 * \see ComboField::AddItem.
 */
struct ComboFieldItem {
  QString name;
  QVariant data;
};

/**
 * @brief The ComboField class
 *
 * An EffectField derivative to produce arbitrary data based on a fixed selection of items.
 */
class ComboField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  ComboField(EffectRow* parent);

  /**
   * @brief Add an item to this ComboField
   *
   * Adds a choice that the user can choose from this ComboField. All choices need text (for the on-screen
   * choice) and data, which gets read on the backend. The selected data is what gets saved and loaded from
   * project files, and therefore the data should be unique to this item. In case more items get added to
   * this ComboField later, old project files will still open correctly. This is also why simple selected
   * indices are not available. The text is only shown on the UI so it can be safely translated during runtime.
   *
   * @param text
   *
   * The text to show at this index.
   *
   * @param data
   *
   * The data to be retrieved at this index.
   */
  void AddItem(const QString& text, const QVariant& data);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a QComboBox with the set of items added in AddItem().
   */
  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

signals:
  /**
   * @brief Signal emitted whenever a connected widget's data gets changed
   *
   * Useful for UI events that need to occur with the change of this ComboField's value.
   */
  void DataChanged(const QVariant&);

private:
  /**
   * @brief Internal array of string+value pair items.
   *
   * \see ComboFieldItem
   */
  QVector<ComboFieldItem> items_;

private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current index of the QWidget (QComboBox in this case). Automatically set when this slot is connected
   * to the QComboBox::currentIndexChanged() signal. This is the only time ComboFields deal with indices since the
   * QComboBox's indices will match precisely to the items_ array. Outside of this function, QVariant data is prefered.
   */
  void UpdateFromWidget(int index);
};

#endif // COMBOFIELD_H
