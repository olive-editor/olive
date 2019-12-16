#ifndef SPEEDDURATIONDIALOG_H
#define SPEEDDURATIONDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QUndoCommand>

#include "node/block/clip/clip.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/timeslider.h"

class SpeedDurationDialog : public QDialog
{
  Q_OBJECT
public:
  SpeedDurationDialog(const rational& timebase, const QList<ClipBlock*>& clips, QWidget* parent = nullptr);

public slots:
  virtual void accept() override;

private:
  QList<ClipBlock*> clips_;

  FloatSlider* speed_slider_;
  TimeSlider* duration_slider_;

  rational timebase_;

  double original_speed_;
  int64_t original_length_;

  QCheckBox* reverse_speed_checkbox_;
  QCheckBox* maintain_audio_pitch_checkbox_;
  QCheckBox* ripple_clips_checkbox_;

private slots:
  void SpeedChanged();
};

#endif // SPEEDDURATIONDIALOG_H
