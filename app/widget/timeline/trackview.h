#ifndef _OLIVE_TIMELINE_WIDGET_TRACK_VIEW_H_
#define _OLIVE_TIMELINE_WIDGET_TRACK_VIEW_H_

#include <QWidget>
#include <set>
#include "widget/timeline/clipview.h"

namespace olive {

class Track;
class Clip;
class SequenceScrollView;
class IThemeService;

class TimelineWidgetTrackView : public QWidget {
  Q_OBJECT

private:
  SequenceScrollView* scrollView_;
  IThemeService* theme_service_;

  Track* track_;

  std::map<int, TimelineWidgetClipView*> clipViews_;

private slots:
  void handleDidAddClip(Clip* const track);
  void handleWillRemoveClip(Clip* const track);

protected:
  void paintEvent(QPaintEvent* event) override;

public:
  TimelineWidgetTrackView(
    QWidget* const parent,
    Track* const track,
    SequenceScrollView* const scrollView,
    IThemeService* const themeService);

  TimelineWidgetClipView* const getClipView(const Clip* const clip);

signals:
  // These signals are kind of a relay which will be emitted when TimelineWidegtClipView emits `onDid...Clip()`
  void onDidFocusClip();
  void onDidFocusBlur();

};

}

#endif