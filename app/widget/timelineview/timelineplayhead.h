#ifndef TIMELINEPLAYHEADSTYLE_H
#define TIMELINEPLAYHEADSTYLE_H

#include <QWidget>

class TimelinePlayhead : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QColor playheadColor READ PlayheadColor WRITE SetPlayheadColor DESIGNABLE true)
  Q_PROPERTY(QColor playheadHighlightColor READ PlayheadHighlightColor WRITE SetPlayheadHighlightColor DESIGNABLE true)
public:
  TimelinePlayhead();

  QColor PlayheadColor();
  QColor PlayheadHighlightColor();

  void SetPlayheadColor(QColor c);
  void SetPlayheadHighlightColor(QColor c);

private:
  QColor playhead_color_;
  QColor playhead_highlight_color_;
};

#endif // TIMELINEPLAYHEADSTYLE_H
