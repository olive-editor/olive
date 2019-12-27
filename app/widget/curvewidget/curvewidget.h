#ifndef CURVEWIDGET_H
#define CURVEWIDGET_H

#include <QPushButton>
#include <QWidget>

#include "curveview.h"

class CurveWidget : public QWidget
{
public:
  CurveWidget(QWidget* parent = nullptr);

private:
  QPushButton* linear_button_;

  QPushButton* bezier_button_;

  QPushButton* hold_button_;

  CurveView* view_;

};

#endif // CURVEWIDGET_H
