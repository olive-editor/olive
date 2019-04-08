#ifndef NODEWIDGET_H
#define NODEWIDGET_H

#include <QWidget>

class NodeUI;

class NodeWidget : public QWidget
{
  Q_OBJECT
public:
  NodeWidget(NodeUI *parent);
protected:
  virtual void resizeEvent(QResizeEvent *event) override;
  virtual bool event(QEvent *event) override;
private:
  NodeUI* parent_;
};

#endif // NODEWIDGET_H
