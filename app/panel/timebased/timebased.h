#ifndef TIMEBASEDPANEL_H
#define TIMEBASEDPANEL_H

#include "widget/panel/panel.h"
#include "widget/timebased/timebased.h"

class TimeBasedPanel : public PanelWidget
{
  Q_OBJECT
public:
  TimeBasedPanel(QWidget *parent = nullptr);

  void ConnectViewerNode(ViewerOutput* node);

  void DisconnectViewerNode();

  rational GetTime();

  ViewerOutput* GetConnectedViewer() const;

  TimeRuler* ruler() const;

  virtual void ZoomIn() override;

  virtual void ZoomOut() override;

  virtual void GoToStart() override;

  virtual void PrevFrame() override;

  virtual void NextFrame() override;

  virtual void GoToEnd() override;

  virtual void GoToPrevCut() override;

  virtual void GoToNextCut() override;

  virtual void PlayPause() override;

  virtual void ShuttleLeft() override;

  virtual void ShuttleStop() override;

  virtual void ShuttleRight() override;

  virtual void SetIn() override;

  virtual void SetOut() override;

  virtual void ResetIn() override;

  virtual void ResetOut() override;

  virtual void ClearInOut() override;

public slots:
  void SetTimebase(const rational& timebase);

  void SetTime(const int64_t& timestamp);

signals:
  void TimeChanged(const int64_t& time);

  void TimebaseChanged(const rational& timebase);

  void PlayPauseRequested();

  void ShuttleLeftRequested();

  void ShuttleStopRequested();

  void ShuttleRightRequested();

protected:
  TimeBasedWidget* GetTimeBasedWidget() const;

  void SetTimeBasedWidget(TimeBasedWidget* widget);

  virtual void Retranslate() override;

private:
  TimeBasedWidget* widget_;

};

#endif // TIMEBASEDPANEL_H
