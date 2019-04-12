#ifndef COMBOINPUT_H
#define COMBOINPUT_H

#include "effects/effectrow.h"

class ComboInput : public EffectRow
{
  Q_OBJECT
public:
  ComboInput(Node* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  /**
   * @brief Add an item to this ComboInput
   *
   * Wrapper for ComboField::AddItem.
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

signals:
  /**
   * @brief Signal emitted whenever a connected widget's data gets changed
   *
   * Wrapper for ComboField::DataChanged.
   */
  void DataChanged(const QVariant&);
};

#endif // COMBOINPUT_H
