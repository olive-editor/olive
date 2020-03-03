#ifndef FOOTAGEVIEWERWIDGET_H
#define FOOTAGEVIEWERWIDGET_H

#include "node/input/media/audio/audio.h"
#include "node/input/media/video/video.h"
#include "node/output/viewer/viewer.h"
#include "viewer.h"

class FootageViewerWidget : public ViewerWidget
{
  Q_OBJECT
public:
  FootageViewerWidget(QWidget* parent = nullptr);

  Footage* GetFootage() const;
  void SetFootage(Footage* footage);

private:
  Footage* footage_;

  VideoInput* video_node_;

  AudioInput* audio_node_;

  ViewerOutput* viewer_node_;

private slots:
  void StartFootageDrag();

};

#endif // FOOTAGEVIEWERWIDGET_H
