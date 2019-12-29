#ifndef NODEPARAMVIEWWIDGETBRIDGE_H
#define NODEPARAMVIEWWIDGETBRIDGE_H

#include <QObject>

#include "node/input.h"
#include "widget/slider/sliderbase.h"

class NodeParamViewWidgetBridge : public QObject
{
  Q_OBJECT
public:
  NodeParamViewWidgetBridge(NodeInput* input, QObject* parent);

  void SetTime(const rational& time);

  const QList<QWidget*>& widgets();

private:
  void CreateWidgets();

  void SetInputValue(const QVariant& value, int track);

  void ProcessSlider(SliderBase* slider, const QVariant& value);

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

};

#endif // NODEPARAMVIEWWIDGETBRIDGE_H
