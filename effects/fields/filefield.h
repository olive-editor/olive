#ifndef FILEFIELD_H
#define FILEFIELD_H

#include "../effectfield.h"

/**
 * @brief The FileField class
 *
 * An EffectField derivative that produces filenames in string and uses an EmbeddedFileChooser
 * as its visual representation.
 */
class FileField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  FileField(EffectRow* parent, const QString& id);

  /**
   * @brief Get the filename at the given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toString()
   *
   * @param timecode
   *
   * The timecode to retrieve the filename at
   *
   * @return
   *
   * The filename at this timecode
   */
  QString GetFileAt(double timecode);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a EmbeddedFileChooser.
   */
  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget *widget, double timecode) override;
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current string of the QWidget (TextEditEx in this case). Automatically set when this slot
   * is connected to the TextEditEx::textModified() signal.
   */
  void UpdateFromWidget(const QString &s);
};

#endif // FILEFIELD_H
