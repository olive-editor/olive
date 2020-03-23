#ifndef DRAGBUTTON_H
#define DRAGBUTTON_H

#include <QPushButton>

class DragButton : public QPushButton
{
  Q_OBJECT
public:
  DragButton(QWidget* parent = nullptr);

signals:
  void MousePressed();

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;

};

#endif // DRAGBUTTON_H
