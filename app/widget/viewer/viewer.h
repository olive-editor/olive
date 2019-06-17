#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

#include "viewerglwidget.h"
#include "widget/playbackcontrols/playbackcontrols.h"

/**
 * @brief The ViewerWidget class
 *
 * An OpenGL-based viewer widget with playback controls.
 */
class ViewerWidget : public QWidget
{
  Q_OBJECT
public:
  ViewerWidget(QWidget* parent);

  void SetPlaybackControlsEnabled(bool enabled);

private:
  ViewerGLWidget* gl_widget_;
  PlaybackControls* controls_;

};

#endif // VIEWER_WIDGET_H
