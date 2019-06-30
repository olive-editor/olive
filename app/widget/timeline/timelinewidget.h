#ifndef _OLIVE_TIMELINE_WIDGET_H_
#define _OLIVE_TIMELINE_WIDGET_H_

#include <QWidget>

namespace olive {

class Sequence;
class SequenceScrollView;
class TimelineWidgetSequenceView;
class IThemeService;

class TimelineWidget : public QWidget {
  Q_OBJECT;

private:
  IThemeService* theme_service_;

  Sequence* sequence_;
  SequenceScrollView* sequence_scroll_view_;
  TimelineWidgetSequenceView* sequence_view_;

protected:
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

public:
  TimelineWidget(QWidget* parent, IThemeService* const themeService);

  void setSequence(Sequence* sequence);

};

}

#endif