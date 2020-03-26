#ifndef NODEPARAMVIEWWIDGETBRIDGE_H
#define NODEPARAMVIEWWIDGETBRIDGE_H

#include <QObject>

#include "node/input.h"
#include "widget/keyframeview/timetargetobject.h"
#include "widget/slider/sliderbase.h"

class NodeParamViewWidgetBridge : public QObject, public TimeTargetObject
{
  Q_OBJECT
public:
  NodeParamViewWidgetBridge(NodeInput* input, QObject* parent);

  void SetTime(const rational& time);

  const QList<QWidget*>& widgets() const;

private:
  void CreateWidgets();

  void SetInputValue(const QVariant& value, int track);

  void ProcessSlider(SliderBase* slider, const QVariant& value);

  void CreateSliders(int count);

  void UpdateWidgetValues();

  rational GetCurrentTimeAsNodeTime() const;

  NodeInput* input_;

  QList<QWidget*> widgets_;

  rational time_;

  bool dragging_;
  bool drag_created_keyframe_;
  QVariant drag_old_value_;
  NodeKeyframePtr dragging_keyframe_;

private slots:
  void WidgetCallback();

  void InputValueChanged(const rational& start, const rational& end);

  void PropertyChanged(const QString& key, const QVariant& value);

};

#endif // NODEPARAMVIEWWIDGETBRIDGE_H
