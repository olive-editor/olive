#ifndef TRACKVIEWSPLITTER_H
#define TRACKVIEWSPLITTER_H

#include <QSplitter>

class TrackViewSplitterHandle : public QSplitterHandle
{
  Q_OBJECT
public:
  TrackViewSplitterHandle(Qt::Orientation orientation, QSplitter *parent);

protected:
  virtual void	mousePressEvent(QMouseEvent *e) override;
  virtual void	mouseMoveEvent(QMouseEvent *e) override;
  virtual void	mouseReleaseEvent(QMouseEvent *e) override;

  virtual void	paintEvent(QPaintEvent *e) override;

private:
  int drag_y_;

  bool dragging_;
};

class TrackViewSplitter : public QSplitter
{
public:
  TrackViewSplitter(Qt::Alignment vertical_alignment, QWidget* parent = nullptr);

  void HandleReceiver(TrackViewSplitterHandle* h, int diff);

protected:
  virtual QSplitterHandle *createHandle() override;

private:
  Qt::Alignment alignment_;
};

#endif // TRACKVIEWSPLITTER_H
