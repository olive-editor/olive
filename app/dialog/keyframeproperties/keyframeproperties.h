#ifndef KEYFRAMEPROPERTIESDIALOG_H
#define KEYFRAMEPROPERTIESDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QGroupBox>

#include "node/keyframe.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/timeslider.h"

class KeyframePropertiesDialog : public QDialog
{
  Q_OBJECT
public:
  KeyframePropertiesDialog(const QList<NodeKeyframePtr>& keys, const rational& timebase, QWidget* parent = nullptr);

public slots:
  virtual void accept() override;

private:
  void SetUpBezierSlider(FloatSlider *slider, bool all_same, double value);

  const QList<NodeKeyframePtr>& keys_;

  rational timebase_;

  TimeSlider* time_slider_;

  QComboBox* type_select_;

  QGroupBox* bezier_group_;

  FloatSlider* bezier_in_x_slider_;

  FloatSlider* bezier_in_y_slider_;

  FloatSlider* bezier_out_x_slider_;

  FloatSlider* bezier_out_y_slider_;

private slots:
  void KeyTypeChanged(int index);

};

#endif // KEYFRAMEPROPERTIESDIALOG_H
