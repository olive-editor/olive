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

#include "viewerwindow.h"

#include <QMutex>
#include <QKeyEvent>
#include <QPainter>
#include <QApplication>
#include <QMenuBar>
#include <QShortcut>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QDebug>

#include "rendering/renderfunctions.h"
#include "rendering/shadergenerators.h"
#include "ui/mainwindow.h"

ViewerWindow::ViewerWindow(QWidget *parent) :
  QOpenGLWidget(parent, Qt::Window),
  texture_(0),
  mutex_(nullptr),
  show_fullscreen_msg_(false)
{
  setMouseTracking(true);

  fullscreen_msg_timer_.setInterval(2000);
  connect(&fullscreen_msg_timer_, SIGNAL(timeout()), this, SLOT(fullscreen_msg_timeout()));
}

void ViewerWindow::set_texture(GLuint t, double iar, QMutex* imutex) {
  texture_ = t;
  ar_ = iar;
  mutex_ = imutex;
  update();
}

void ViewerWindow::shortcut_copier(QVector<QShortcut*>& shortcuts, QMenu* menu) {
  QList<QAction*> menu_action = menu->actions();
  for (int i=0;i<menu_action.size();i++) {
    if (menu_action.at(i)->menu() != nullptr) {
      shortcut_copier(shortcuts, menu_action.at(i)->menu());
    } else if (!menu_action.at(i)->isSeparator() && !menu_action.at(i)->shortcut().isEmpty()) {
      QShortcut* sc = new QShortcut(this);
      sc->setKey(menu_action.at(i)->shortcut());
      connect(sc, SIGNAL(activated()), menu_action.at(i), SLOT(trigger()));
      shortcuts.append(sc);
    }
  }
}

void ViewerWindow::showEvent(QShowEvent *)
{
  // Here, we copy all shortcuts from the MainWindow to this window. I don't like this solution, but messing around
  // with Qt's event system proved fruitless. Also setting the shortcuts to ApplicationShortcut rather than
  // WindowShortcut caused issues elsewhere (shortcuts being picked up in comboboxes and dialog boxes - we only
  // want the shortcuts to be shared to this window). Therefore, this and shortcut_copier() are so far the best
  // solutions I can find.

  // Clear any existing shortcuts in case they've changed since the last showing
  for (int i=0;i<shortcuts_.size();i++) {
    delete shortcuts_.at(i);
  }
  shortcuts_.clear();

  // Recursively copy all shortcuts from MainWindow to this window
  QList<QAction*> menubar_actions = olive::MainWindow->menuBar()->actions();
  for (int i=0;i<menubar_actions.size();i++) {
    shortcut_copier(shortcuts_, menubar_actions.at(i)->menu());
  }
}

void ViewerWindow::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key_Escape) {
    hide();
  }
}

void ViewerWindow::mousePressEvent(QMouseEvent *e) {
  if (show_fullscreen_msg_ && fullscreen_msg_rect_.contains(e->pos())) {
    hide();
  }
}

void ViewerWindow::mouseMoveEvent(QMouseEvent *) {
  fullscreen_msg_timer_.start();
  if (!show_fullscreen_msg_) {
    show_fullscreen_msg_ = true;
    update();
  }
}

void ViewerWindow::initializeGL()
{
  pipeline_ = olive::shader::GetPipeline();
}

void ViewerWindow::paintGL() {
  if (texture_ > 0) {
    if (mutex_ != nullptr) mutex_->lock();


    QOpenGLFunctions* f = context()->functions();
    //QOpenGLExtraFunctions* xf = context()->extraFunctions();

    makeCurrent();

    // clear to solid black
    f->glClearColor(0.0, 0.0, 0.0, 0.0);
    f->glClear(GL_COLOR_BUFFER_BIT);


    // draw texture from render thread


    QMatrix4x4 matrix;

    double widget_ar = (double(width()) / double(height()));
    if (widget_ar > ar_) {
      matrix.scale(ar_ / widget_ar, 1.0);
    } else {
      matrix.scale(1.0f, widget_ar / ar_);
    }


    f->glViewport(0, 0, width(), height());

    f->glBindTexture(GL_TEXTURE_2D, texture_);

    olive::rendering::Blit(pipeline_.get(), true, matrix);

    f->glBindTexture(GL_TEXTURE_2D, 0);



    if (mutex_ != nullptr) mutex_->unlock();
  }

  if (show_fullscreen_msg_) {
    QPainter p(this);

    QFont f = p.font();
    f.setPointSize(24);
    p.setFont(f);

    QFontMetrics fm(f);

    QString fs_str = tr("Exit Fullscreen");

    p.setPen(Qt::white);
    p.setBrush(QColor(0, 0, 0, 128));

    int text_width = fm.width(fs_str);
    int text_x = (width()/2)-(text_width/2);
    int text_y = fm.height()+fm.ascent();

    int rect_padding = 8;

    fullscreen_msg_rect_ = QRect(text_x-rect_padding,
                                fm.height()-rect_padding,
                                text_width+rect_padding+rect_padding,
                                fm.height()+rect_padding+rect_padding);

    p.drawRect(fullscreen_msg_rect_);

    p.drawText(text_x, text_y, fs_str);
  }
}

void ViewerWindow::fullscreen_msg_timeout() {
  fullscreen_msg_timer_.stop();
  if (show_fullscreen_msg_) {
    show_fullscreen_msg_ = false;
    update();
  }
}
