#include "curvewidget.h"

#include <QLabel>
#include <QVBoxLayout>

#include "widget/nodeparamview/nodeparamviewkeyframecontrol.h"

CurveWidget::CurveWidget(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QHBoxLayout* top_controls = new QHBoxLayout();

  NodeParamViewKeyframeControl* key_controls = new NodeParamViewKeyframeControl(false);
  key_controls->SetEnableButtonVisible(false);
  key_controls->SetPreviousButtonEnabled(false);
  key_controls->SetToggleButtonEnabled(false);
  key_controls->SetNextButtonEnabled(false);
  top_controls->addWidget(key_controls);

  top_controls->addStretch();

  linear_button_ = new QPushButton(tr("Linear"));
  linear_button_->setCheckable(true);
  linear_button_->setEnabled(false);
  top_controls->addWidget(linear_button_);

  bezier_button_ = new QPushButton(tr("Bezier"));
  bezier_button_->setCheckable(true);
  bezier_button_->setEnabled(false);
  top_controls->addWidget(bezier_button_);

  hold_button_ = new QPushButton(tr("Hold"));
  hold_button_->setCheckable(true);
  hold_button_->setEnabled(false);
  top_controls->addWidget(hold_button_);

  layout->addLayout(top_controls);

  view_ = new CurveView();
  layout->addWidget(view_);

  layout->addWidget(new QLabel("Values"));
}
