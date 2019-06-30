#ifndef _OLIVE_TIMELINE_WIDGET_CLIP_VIEW_H_
#define _OLIVE_TIMELINE_WIDGET_CLIP_VIEW_H_

#include <QWidget>

namespace olive {

class Clip;
class SequenceScrollView;
class IThemeService;

class TimelineWidgetClipView : public QWidget {
  Q_OBJECT

private:
  Clip* clip_;
  SequenceScrollView* scrollView_;

  bool focused_;

  IThemeService* theme_service_;

  void updateView();

protected:
  void mousePressEvent(QMouseEvent * event) override;

  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

public:
  TimelineWidgetClipView(
    QWidget* const parent,
    Clip* const clip,
    SequenceScrollView* const scrollView,
    IThemeService* const themeService);

  void focus();
  void blur();
  bool focused() const;

signals:
  void onDidFocus();
  void onDidBlur();

};

}

#endif