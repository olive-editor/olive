/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QOpenGLWidget>
#include <QTimer>
#include <QMutex>
#include <QMenu>

#include "rendering/qopenglshaderprogramptr.h"

class ViewerWindow : public QOpenGLWidget {
  Q_OBJECT
public:
  ViewerWindow(QWidget *parent);
  void set_texture(GLuint t, double iar, QMutex *imutex);
protected:
  virtual void showEvent(QShowEvent*) override;
  virtual void keyPressEvent(QKeyEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;

  virtual void initializeGL() override;
  virtual void paintGL() override;
private:
  GLuint texture_;
  double ar_;
  QMutex* mutex_;
  QOpenGLShaderProgramPtr pipeline_;

  // shortcuts
  void shortcut_copier(QVector<QShortcut*>& shortcuts, QMenu* menu);
  QVector<QShortcut*> shortcuts_;

  // exit full screen message
  QTimer fullscreen_msg_timer_;
  bool show_fullscreen_msg_;
  QRect fullscreen_msg_rect_;
private slots:
  void fullscreen_msg_timeout();
};

#endif // VIEWERWINDOW_H
