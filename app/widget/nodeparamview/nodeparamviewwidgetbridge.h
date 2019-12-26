#ifndef NODEPARAMVIEWWIDGETBRIDGE_H
#define NODEPARAMVIEWWIDGETBRIDGE_H

#include <QObject>

#include "node/input.h"

class NodeParamViewWidgetBridge : public QObject
{
  Q_OBJECT
public:
  NodeParamViewWidgetBridge(NodeInput* input, QObject* parent);

  void SetTime(const rational& time);

  const QList<QWidget*>& widgets();

private:
  void CreateWidgets();

  void SetInputValue(const QVariant& value);

  NodeInput* input_;

  QList<QWidget*> widgets_;

  rational time_;

private slots:
  void WidgetCallback();
};

#endif // NODEPARAMVIEWWIDGETBRIDGE_H
