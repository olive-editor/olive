#ifndef RESIZABLESCROLLBAR_H
#define RESIZABLESCROLLBAR_H

#include <QScrollBar>

class ResizableScrollBar : public QScrollBar
{
  Q_OBJECT
public:
  ResizableScrollBar(QWidget* parent = nullptr);
  ResizableScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

signals:
  void RequestScale(const double& multiplier);

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;

  virtual void mouseMoveEvent(QMouseEvent* event) override;

  virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:
  static const int kHandleWidth;

  enum MouseHandleState {
    kNotInHandle,
    kInTopHandle,
    kInBottomHandle
  };

  void Init();

  int GetActiveMousePos(QMouseEvent* event);

  MouseHandleState mouse_handle_state_;

  bool mouse_dragging_;

  int mouse_drag_start_;

};

#endif // RESIZABLESCROLLBAR_H
