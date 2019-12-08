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
  Q_OBJECT
public:
  TrackViewSplitter(Qt::Alignment vertical_alignment, QWidget* parent = nullptr);

  void HandleReceiver(TrackViewSplitterHandle* h, int diff);

  void SetHeightWithSizes(const QList<int>& sizes);

  void Insert(int index, int height, QWidget* item);
  void Remove(int index);

public slots:
  void SetTrackHeight(int index, int h);

signals:
  void TrackHeightChanged(int index, int height);

protected:
  virtual QSplitterHandle *createHandle() override;

private:
  Qt::Alignment alignment_;
};

#endif // TRACKVIEWSPLITTER_H
