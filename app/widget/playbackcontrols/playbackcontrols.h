#ifndef PLAYBACKCONTROLS_H
#define PLAYBACKCONTROLS_H

#include <QWidget>
#include <QLabel>

class PlaybackControls : public QWidget
{
public:
  PlaybackControls(QWidget* parent);

  void SetTimecodeEnabled(bool enabled);

signals:
  void BeginClicked();
  void PrevFrameClicked();
  void PlayClicked();
  void NextFrameClicked();
  void EndClicked();

private:
  QWidget* lower_left_container_;
  QWidget* lower_right_container_;

  QLabel* cur_tc_lbl_;
  QLabel* end_tc_lbl_;
};

#endif // PLAYBACKCONTROLS_H
