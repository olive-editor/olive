#ifndef STRINGFIELD_H
#define STRINGFIELD_H

#include "../effectfield.h"

/**
 * @brief The StringField class
 *
 * An EffectField derivative that produces arbitrary strings entered by the user and uses a TextEditEx as its
 * visual representation.
 */
class StringField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   *
   * Provides a setting for whether this StringField - and its attached TextEditEx objects - should operate in rich
   * text or plain text mode, defaulting to rich text mode.
   */
  StringField(EffectRow* parent, const QString& id, bool rich_text = true);

  /**
   * @brief Get the string at the given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toString()
   *
   * @param timecode
   *
   * The timecode to retrieve the string at
   *
   * @return
   *
   * The string at this timecode
   */
  QString GetStringAt(double timecode);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a TextEditEx.
   */
  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current checked state of the QWidget (EmbeddedFileChooser in this case). Automatically set when this slot
   * is connected to the EmbeddedFileChooser::changed() signal.
   */
  void UpdateFromWidget(const QString& b);
private:
  /**
   * @brief Internal value for whether this field is in rich text or plain text mode
   */
  bool rich_text_;
};

#endif // STRINGFIELD_H
