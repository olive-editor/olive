#ifndef CLIPPROPERTIESDIALOG_H
#define CLIPPROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "timeline/clip.h"
#include "ui/labelslider.h"

class ClipPropertiesDialog : public QDialog {
    Q_OBJECT
public:
  ClipPropertiesDialog(QWidget* parent, QVector<Clip*> clips);
protected:
  virtual void accept() override;
private:
  QVector<Clip*> clips_;

  QLineEdit* clip_name_field_;
  LabelSlider* duration_field_;
};

#endif // CLIPPROPERTIESDIALOG_H
