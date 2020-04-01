#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QWidget>

#include "viewerglwidget.h"

class ViewerWindow : public QWidget
{
public:
  ViewerWindow(QWidget* parent = nullptr);

  ViewerGLWidget* gl_widget() const;

  /**
   * @brief Used to adjust resulting picture to be the right aspect ratio
   */
  void SetResolution(int width, int height);

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;

  virtual void closeEvent(QCloseEvent* e) override;

private:
  ViewerGLWidget* gl_widget_;

};

#endif // VIEWERWINDOW_H
