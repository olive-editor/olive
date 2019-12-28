#ifndef CURVEWIDGET_H
#define CURVEWIDGET_H

#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>

#include "curveview.h"
#include "node/input.h"
#include "widget/nodeparamview/nodeparamviewkeyframecontrol.h"
#include "widget/nodeparamview/nodeparamviewwidgetbridge.h"
#include "widget/timeruler/timeruler.h"

class CurveWidget : public QWidget
{
  Q_OBJECT
public:
  CurveWidget(QWidget* parent = nullptr);

  virtual ~CurveWidget() override;

  void SetInput(NodeInput* input);

  void SetTimebase(const rational& timebase);

  void SetTime(const int64_t& timestamp);

  const double& GetScale();
  void SetScale(const double& scale);

signals:
  void TimeChanged(const int64_t& timestamp);

protected:
  virtual void changeEvent(QEvent *) override;

private:
  void UpdateInputLabel();

  void SetKeyframeButtonEnabled(bool enable);

  void SetKeyframeButtonChecked(bool checked);

  void SetKeyframeButtonCheckedFromType(NodeKeyframe::Type type);

  QPushButton* linear_button_;

  QPushButton* bezier_button_;

  QPushButton* hold_button_;

  TimeRuler* ruler_;

  CurveView* view_;

  NodeInput* input_;

  QLabel* input_label_;

  QHBoxLayout* widget_bridge_layout_;

  NodeParamViewWidgetBridge* bridge_;

  NodeParamViewKeyframeControl* key_control_;

private slots:
  void UpdateBridgeTime(const int64_t& timestamp);

  void SelectionChanged();

  void KeyframeTypeButtonTriggered(bool checked);

  void KeyControlRequestedTimeChanged(const rational& time);

};

#endif // CURVEWIDGET_H
