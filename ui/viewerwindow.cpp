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
  pipeline_ = olive::rendering::GetPipeline();
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
