#ifndef AUDIOMONITORWIDGET_H
#define AUDIOMONITORWIDGET_H

#include <QTimer>
#include <QWidget>

class AudioMonitor : public QWidget
{
  Q_OBJECT
public:
  AudioMonitor(QWidget* parent = nullptr);

public slots:
  void SetValues(QVector<double> values);
  void Clear();

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;

private:
  QVector<double> values_;
  QVector<bool> peaked_;

  QTimer clear_timer_;
};

#endif // AUDIOMONITORWIDGET_H
