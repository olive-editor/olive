#ifndef CLIPPROPERTIESDIALOG_H
#define CLIPPROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "timeline/clip.h"
#include "ui/labelslider.h"

/**
 * @brief The ClipPropertiesDialog class
 *
 * A dialog for setting Clip properties, accessible by right clicking a Clip and clicking "Properties". This can be
 * run from anywhere provided it's given a valid array of Clip objects.
 */
class ClipPropertiesDialog : public QDialog {
    Q_OBJECT
public:
  /**
   * @brief ClipPropertiesDialog Constructor
   * @param parent
   *
   * Parent widget.
   *
   * @param clips
   *
   * Array of Clip objects to set the properties of.
   */
  ClipPropertiesDialog(QWidget* parent, QVector<Clip*> clips);
protected:
  /**
   * @brief Accept override. Saves the current properties to the array of Clips.
   */
  virtual void accept() override;
private:
  /**
   * @brief Internal clip array (set in the constructor)
   */
  QVector<Clip*> clips_;

  /**
   * @brief Widget for setting the Clip names
   */
  QLineEdit* clip_name_field_;

  /**
   * @brief Widget for setting the Clip durations
   */
  LabelSlider* duration_field_;
};

#endif // CLIPPROPERTIESDIALOG_H
