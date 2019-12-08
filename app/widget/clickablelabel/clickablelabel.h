#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

class ClickableLabel : public QLabel
{
  Q_OBJECT
public:
  ClickableLabel(const QString& text, QWidget* parent = nullptr);
  ClickableLabel(QWidget* parent = nullptr);

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

signals:
  void MouseClicked();
  void MouseDoubleClicked();

};

#endif // CLICKABLELABEL_H
