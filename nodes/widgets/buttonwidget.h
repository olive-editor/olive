#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include "nodes/nodeio.h"

class ButtonWidget : public NodeIO
{
public:
  ButtonWidget(OldEffectNode* parent, const QString& name, const QString& text);

  /**
   * @brief Wrapper for ButtonField::SetCheckable.
   */
  void SetCheckable(bool c);

public slots:
  /**
   * @brief Wrapper for ButtonField::SetChecked()
   */
  void SetChecked(bool c);

signals:
  /**
   * @brief Wrapper for ButtonField::CheckedChanged()
   */
  void CheckedChanged(bool);

  /**
   * @brief Wrapper for ButtonField::Toggled()
   */
  void Toggled(bool);

};

#endif // BUTTONWIDGET_H
