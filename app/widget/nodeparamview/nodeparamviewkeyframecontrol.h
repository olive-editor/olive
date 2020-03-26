#ifndef NODEPARAMVIEWKEYFRAMECONTROL_H
#define NODEPARAMVIEWKEYFRAMECONTROL_H

#include <QPushButton>
#include <QWidget>

#include "node/input.h"
#include "widget/keyframeview/timetargetobject.h"

class NodeParamViewKeyframeControl : public QWidget, public TimeTargetObject
{
  Q_OBJECT
public:
  NodeParamViewKeyframeControl(bool right_align = true, QWidget* parent = nullptr);

  NodeInput* GetConnectedInput() const;

  void SetInput(NodeInput* input);

  void SetTime(const rational& time);

signals:
  void RequestSetTime(const rational& time);

private:
  QPushButton* CreateNewToolButton(const QIcon &icon) const;

  void SetButtonsEnabled(bool e);

  rational GetCurrentTimeAsNodeTime() const;

  rational ConvertToViewerTime(const rational& r) const;

  QPushButton* prev_key_btn_;
  QPushButton* toggle_key_btn_;
  QPushButton* next_key_btn_;
  QPushButton* enable_key_btn_;

  NodeInput* input_;

  rational time_;

private slots:
  void ShowButtonsFromKeyframeEnable(bool e);

  void ToggleKeyframe(bool e);

  void UpdateState();

  void GoToPreviousKey();

  void GoToNextKey();

  void KeyframeEnableChanged(bool e);

};

#endif // NODEPARAMVIEWKEYFRAMECONTROL_H
