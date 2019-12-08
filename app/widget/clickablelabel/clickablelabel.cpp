#include "clickablelabel.h"

ClickableLabel::ClickableLabel(const QString &text, QWidget *parent) :
  QLabel(text, parent)
{
}

ClickableLabel::ClickableLabel(QWidget *parent) :
  QLabel(parent)
{
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent *)
{
  if (underMouse()) {
    emit MouseClicked();
  }
}

void ClickableLabel::mouseDoubleClickEvent(QMouseEvent *)
{
  emit MouseDoubleClicked();
}
