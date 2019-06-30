#ifndef _OLIVE_TIMELINE_WIDGET_SEQUENCE_VIEW_H_
#define _OLIVE_TIMELINE_WIDGET_SEQUENCE_VIEW_H_

#include <QWidget>
#include <vector>

namespace olive {

class Track;
class Sequence;
class TimelineWidgetTrackView;
class SequenceScrollView;
class IThemeService;

class TimelineWidgetSequenceView : public QWidget {
  Q_OBJECT

private:
  Sequence* sequence_;
  SequenceScrollView* scrollView_;

  std::vector<TimelineWidgetTrackView*> track_views_;

  IThemeService* theme_service_;
  
private slots:
  void handleDidAddTrack(Track* const track, int index);
  void handleWillRemoveTrack(Track* const track, int index);

protected:
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

public:
  TimelineWidgetSequenceView(
    QWidget* parent,
    Sequence* const sequence,
    SequenceScrollView* const scrollView,
    IThemeService* const themeService);

};

}

#endif