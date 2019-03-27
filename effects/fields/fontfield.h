#ifndef FONTFIELD_H
#define FONTFIELD_H

#include "combofield.h"

/**
 * @brief The FontField class
 *
 * An EffectField derivative the produces font family names in string and uses a QComboBox
 * as its visual representation.
 *
 * TODO Upgrade to QFontComboBox.
 */
class FontField : public EffectField {
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  FontField(EffectRow* parent, const QString& id);

  /**
   * @brief Get the font family name at the given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toString()
   *
   * @param timecode
   *
   * The timecode to retrieve the font family name at
   *
   * @return
   *
   * The font family name at this timecode
   */
  QString GetFontAt(double timecode);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a QComboBox.
   */
  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

private:
  /**
   * @brief Internal list of fonts to add to a QComboBox when creating one in CreateWidget().
   *
   * NOTE: Deprecated. Once QComboBox is replaced by QFontComboBox this will be completely unnecessary.
   */
  QStringList font_list;
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current font name specified by the QWidget (QComboBox in this case). Automatically set when this slot
   * is connected to the QComboBox::currentTextChanged() signal.
   */
  void UpdateFromWidget(const QString& index);
};

#endif // FONTFIELD_H
