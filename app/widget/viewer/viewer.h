#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

#include "viewerglwidget.h"

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

private:
  ViewerGLWidget* gl_widget_;

  QLabel* cur_tc_lbl_;
  QLabel* end_tc_lbl_;
};

#endif // VIEWER_WIDGET_H
