#ifndef CURVEWIDGET_H
#define CURVEWIDGET_H

#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>

#include "curveview.h"
#include "node/input.h"
#include "widget/nodeparamview/nodeparamviewkeyframecontrol.h"
#include "widget/nodeparamview/nodeparamviewwidgetbridge.h"
#include "widget/timebased/timebased.h"

class CurveWidget : public TimeBasedWidget, public TimeTargetObject
{
  Q_OBJECT
public:
  CurveWidget(QWidget* parent = nullptr);

  virtual ~CurveWidget() override;

  void SetInput(NodeInput* input);

  const double& GetVerticalScale();
  void SetVerticalScale(const double& vscale);

protected:
  virtual void changeEvent(QEvent *) override;

  virtual void TimeChangedEvent(const int64_t &) override;
  virtual void TimebaseChangedEvent(const rational &) override;
  virtual void ScaleChangedEvent(const double &) override;

  virtual void TimeTargetChangedEvent(Node* target) override;

private:
  void UpdateInputLabel();

  void SetKeyframeButtonEnabled(bool enable);

  void SetKeyframeButtonChecked(bool checked);

  void SetKeyframeButtonCheckedFromType(NodeKeyframe::Type type);

  void UpdateBridgeTime(const int64_t& timestamp);

  QPushButton* linear_button_;

  QPushButton* bezier_button_;

  QPushButton* hold_button_;

  CurveView* view_;

  NodeInput* input_;

  QLabel* input_label_;

  QHBoxLayout* widget_bridge_layout_;

  NodeParamViewWidgetBridge* bridge_;

  NodeParamViewKeyframeControl* key_control_;

private slots:
  void SelectionChanged();

  void KeyframeTypeButtonTriggered(bool checked);

  void KeyControlRequestedTimeChanged(const rational& time);

};

#endif // CURVEWIDGET_H
