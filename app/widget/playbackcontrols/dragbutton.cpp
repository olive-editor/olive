#include "dragbutton.h"

DragButton::DragButton(QWidget *parent) :
  QPushButton(parent)
{
}

void DragButton::mousePressEvent(QMouseEvent *event) {
  QPushButton::mousePressEvent(event);

  emit MousePressed();
}
