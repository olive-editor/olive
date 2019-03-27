#ifndef LABELFIELD_H
#define LABELFIELD_H

#include "../effectfield.h"

/**
 * @brief The LabelField class
 *
 * A UI-type EffectField. This field is largely an EffectField wrapper around a QLabel and provides no data that's
 * usable in the Effect. It's primarily useful for showing UI information. This field is not exposed to the external
 * shader API as it requires raw C++ code to connect it to other elements.
 */
class LabelField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  LabelField(EffectRow* parent, const QString& string);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a QLabel.
   */
  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
private:
  /**
   * @brief Internal text string
   */
  QString label_text_;
};

#endif // LABELFIELD_H
