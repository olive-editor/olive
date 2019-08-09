#ifndef NODEPARAMVIEWWIDGETBRIDGE_H
#define NODEPARAMVIEWWIDGETBRIDGE_H

#include <QObject>

#include "node/input.h"

class NodeParamViewWidgetBridge : public QObject
{
  Q_OBJECT
public:
  NodeParamViewWidgetBridge(QObject* parent);

  void AddInput(NodeInput* input);

  const QList<QWidget*>& widgets();

private:
  QList<NodeInput*> inputs_;

  QList<QWidget*> widgets_;

  void CreateWidgets();

  rational Now();

private slots:
  void WidgetCallback();
};

#endif // NODEPARAMVIEWWIDGETBRIDGE_H
