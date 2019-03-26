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

#include "viewerwidget.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <QPainter>
#include <QAudioOutput>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QtMath>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QOffscreenSurface>
#include <QFileDialog>
#include <QPolygon>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QOpenGLBuffer>

#include "panels/panels.h"
#include "project/projectelements.h"
#include "rendering/renderfunctions.h"
#include "rendering/audio.h"
#include "global/config.h"
#include "global/debug.h"
#include "global/math.h"
#include "global/timing.h"
#include "ui/collapsiblewidget.h"
#include "undo/undo.h"
#include "project/media.h"
#include "ui/viewercontainer.h"
#include "rendering/cacher.h"
#include "ui/timelinewidget.h"
#include "rendering/renderfunctions.h"
#include "rendering/renderthread.h"
#include "rendering/shadergenerators.h"
#include "ui/viewerwindow.h"
#include "ui/menu.h"
#include "mainwindow.h"

const int kTitleActionSafeVertexSize = 84;

ViewerWidget::ViewerWidget(QWidget *parent) :
  QOpenGLWidget(parent),
  waveform(false),
  waveform_zoom(1.0),
  waveform_scroll(0),
  dragging(false),
  gizmos(nullptr),
  selected_gizmo(nullptr),
  x_scroll(0),
  y_scroll(0)
{
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));

  renderer.start(QThread::HighestPriority);
  connect(&renderer, SIGNAL(ready()), this, SLOT(queue_repaint()));

  window = new ViewerWindow(this);
}

ViewerWidget::~ViewerWidget() {
  renderer.cancel();
}

void ViewerWidget::set_waveform_scroll(int s) {
  if (waveform) {
    waveform_scroll = s;
    update();
  }
}

void ViewerWidget::set_fullscreen(int screen) {
  if (screen >= 0 && screen < QGuiApplication::screens().size()) {
    QScreen* selected_screen = QGuiApplication::screens().at(screen);
    window->showFullScreen();
    window->setGeometry(selected_screen->geometry());
  } else {
    qCritical() << "Failed to find requested screen" << screen << "to set fullscreen to";
  }
}

void ViewerWidget::show_context_menu() {
  Menu menu(this);

  QAction* save_frame_as_image = menu.addAction(tr("Save Frame as Image..."));
  connect(save_frame_as_image, SIGNAL(triggered(bool)), this, SLOT(save_frame()));

  Menu* fullscreen_menu = new Menu(tr("Show Fullscreen"));
  menu.addMenu(fullscreen_menu);
  QList<QScreen*> screens = QGuiApplication::screens();
  if (window->isVisible()) {
    fullscreen_menu->addAction(tr("Disable"));
  }
  for (int i=0;i<screens.size();i++) {
    QAction* screen_action = fullscreen_menu->addAction(tr("Screen %1: %2x%3").arg(
                                                          QString::number(i),
                                                          QString::number(screens.at(i)->size().width()),
                                                          QString::number(screens.at(i)->size().height())));
    screen_action->setData(i);
  }
  connect(fullscreen_menu, SIGNAL(triggered(QAction*)), this, SLOT(fullscreen_menu_action(QAction*)));

  Menu zoom_menu(tr("Zoom"));
  QAction* fit_zoom = zoom_menu.addAction(tr("Fit"));
  connect(fit_zoom, SIGNAL(triggered(bool)), this, SLOT(set_fit_zoom()));
  zoom_menu.addAction("10%")->setData(0.1);
  zoom_menu.addAction("25%")->setData(0.25);
  zoom_menu.addAction("50%")->setData(0.5);
  zoom_menu.addAction("75%")->setData(0.75);
  zoom_menu.addAction("100%")->setData(1.0);
  zoom_menu.addAction("150%")->setData(1.5);
  zoom_menu.addAction("200%")->setData(2.0);
  zoom_menu.addAction("400%")->setData(4.0);
  QAction* custom_zoom = zoom_menu.addAction(tr("Custom"));
  connect(custom_zoom, SIGNAL(triggered(bool)), this, SLOT(set_custom_zoom()));
  connect(&zoom_menu, SIGNAL(triggered(QAction*)), this, SLOT(set_menu_zoom(QAction*)));
  menu.addMenu(&zoom_menu);

  if (!viewer->is_main_sequence()) {
    menu.addAction(tr("Close Media"), viewer, SLOT(close_media()));
  }

  menu.exec(QCursor::pos());
}

void ViewerWidget::save_frame() {
  QFileDialog fd(this);
  fd.setAcceptMode(QFileDialog::AcceptSave);
  fd.setFileMode(QFileDialog::AnyFile);
  fd.setWindowTitle(tr("Save Frame"));
  fd.setNameFilter("Portable Network Graphic (*.png);;JPEG (*.jpg);;Windows Bitmap (*.bmp);;Portable Pixmap (*.ppm);;X11 Bitmap (*.xbm);;X11 Pixmap (*.xpm)");

  if (fd.exec()) {
    QString fn = fd.selectedFiles().at(0);
    QString selected_ext = fd.selectedNameFilter().mid(fd.selectedNameFilter().indexOf(QRegExp("\\*.[a-z][a-z][a-z]")) + 1, 4);
    if (!fn.endsWith(selected_ext,  Qt::CaseInsensitive)) {
      fn += selected_ext;
    }

    renderer.start_render(context(), viewer->seq.get(), 1, fn);
  }
}

void ViewerWidget::queue_repaint() {
  update();
}

void ViewerWidget::fullscreen_menu_action(QAction *action) {
  if (action->data().isNull()) {
    window->hide();
  } else {
    set_fullscreen(action->data().toInt());
  }
}

void ViewerWidget::set_fit_zoom() {
  container->fit = true;
  container->adjust();
}

void ViewerWidget::set_custom_zoom() {
  bool ok;
  double d = QInputDialog::getDouble(this,
                                     tr("Viewer Zoom"),
                                     tr("Set Custom Zoom Value:"),
                                     container->zoom*100, 0, 2147483647, 2, &ok);
  if (ok) {
    container->fit = false;
    container->zoom = d*0.01;
    container->adjust();
  }
}

void ViewerWidget::set_menu_zoom(QAction* action) {
  const QVariant& data = action->data();
  if (!data.isNull()) {
    container->fit = false;
    container->zoom = data.toDouble();
    container->adjust();
  }
}

void ViewerWidget::retry() {
  update();
}

void ViewerWidget::initializeGL() {
  context()->functions()->initializeOpenGLFunctions();

  connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(context_destroy()), Qt::DirectConnection);

  pipeline_ = olive::shader::GetPipeline();

  vao_.create();

  title_safe_area_buffer_.create();
  title_safe_area_buffer_.bind();
  title_safe_area_buffer_.allocate(nullptr, kTitleActionSafeVertexSize * sizeof(GLfloat));
  title_safe_area_buffer_.release();

  gizmo_buffer_.create();
}

void ViewerWidget::frame_update() {
  if (viewer->seq != nullptr) {
    // send context to other thread for drawing
    if (waveform) {
      update();
    } else {
      doneCurrent();
      renderer.start_render(context(), viewer->seq.get(), viewer->get_playback_speed());
    }

    // render the audio
    olive::rendering::compose_audio(viewer, viewer->seq.get(), viewer->get_playback_speed(), viewer->WaitingForPlayWake());
  }
}

RenderThread *ViewerWidget::get_renderer() {
  return &renderer;
}

void ViewerWidget::set_scroll(double x, double y) {
  x_scroll = x;
  y_scroll = y;
  update();
}

void ViewerWidget::seek_from_click(int x) {
  viewer->seek(getFrameFromScreenPoint(waveform_zoom, x+waveform_scroll));
}

QMatrix4x4 ViewerWidget::get_matrix()
{
  QMatrix4x4 matrix;

  double zoom_factor = container->zoom/(double(width())/double(viewer->seq->width));

  if (zoom_factor > 1.0) {
    double zoom_size = (zoom_factor*2.0) - 2.0;

    matrix.translate((-(x_scroll-0.5))*zoom_size, (y_scroll-0.5)*zoom_size);
    matrix.scale(zoom_factor);
  }

  return matrix;
}

void ViewerWidget::context_destroy() {
  makeCurrent();

  renderer.delete_ctx();

  title_safe_area_buffer_.destroy();

  if (gizmo_buffer_.isCreated()) {
    gizmo_buffer_.destroy();
  }

  vao_.destroy();

  pipeline_ = nullptr;

  doneCurrent();
}

EffectGizmo* ViewerWidget::get_gizmo_from_mouse(int x, int y) {
  if (gizmos != nullptr) {
    double multiplier = double(viewer->seq->width) / double(width());
    QPoint mouse_pos(qRound(x*multiplier), qRound((height()-y)*multiplier));
    int dot_size = 2 * qRound(GIZMO_DOT_SIZE * multiplier);
    int target_size = 2 * qRound(GIZMO_TARGET_SIZE * multiplier);
    for (int i=0;i<gizmos->gizmo_count();i++) {
      EffectGizmo* g = gizmos->gizmo(i);

      switch (g->get_type()) {
      case GIZMO_TYPE_DOT:
        if (mouse_pos.x() > g->screen_pos[0].x() - dot_size
            && mouse_pos.y() > g->screen_pos[0].y() - dot_size
            && mouse_pos.x() < g->screen_pos[0].x() + dot_size
            && mouse_pos.y() < g->screen_pos[0].y() + dot_size) {
          return g;
        }
        break;
      case GIZMO_TYPE_POLY:
        if (QPolygon(g->screen_pos).containsPoint(mouse_pos, Qt::OddEvenFill)) {
          return g;
        }
        break;
      case GIZMO_TYPE_TARGET:
        if (mouse_pos.x() > g->screen_pos[0].x() - target_size
            && mouse_pos.y() > g->screen_pos[0].y() - target_size
            && mouse_pos.x() < g->screen_pos[0].x() + target_size
            && mouse_pos.y() < g->screen_pos[0].y() + target_size) {
          return g;
        }
        break;
      }
    }
  }
  return nullptr;
}

void ViewerWidget::move_gizmos(QMouseEvent *event, bool done) {
  if (selected_gizmo != nullptr) {
    double multiplier = double(viewer->seq->width) / double(width());

    int x_movement = qRound((event->pos().x() - drag_start_x)*multiplier);
    int y_movement = qRound((event->pos().y() - drag_start_y)*multiplier);

    gizmos->gizmo_move(selected_gizmo, x_movement, y_movement, get_timecode(gizmos->parent_clip, gizmos->parent_clip->sequence->playhead), done);

    gizmo_x_mvmt += x_movement;
    gizmo_y_mvmt += y_movement;

    drag_start_x = event->pos().x();
    drag_start_y = event->pos().y();
  }
}

void ViewerWidget::mousePressEvent(QMouseEvent* event) {
  if (waveform) {
    seek_from_click(event->x());
  } else if (event->buttons() & Qt::MiddleButton || panel_timeline->tool == TIMELINE_TOOL_HAND) {
    container->dragScrollPress(event->pos()*container->zoom);
  } else if (event->buttons() & Qt::LeftButton) {
    drag_start_x = event->pos().x();
    drag_start_y = event->pos().y();

    gizmo_x_mvmt = 0;
    gizmo_y_mvmt = 0;

    selected_gizmo = get_gizmo_from_mouse(event->pos().x(), event->pos().y());
  }
  dragging = true;
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event) {
  unsetCursor();
  if (panel_timeline->tool == TIMELINE_TOOL_HAND) {
    setCursor(Qt::OpenHandCursor);
  }
  if (dragging) {
    if (waveform) {
      seek_from_click(event->x());
    } else if (event->buttons() & Qt::MiddleButton || panel_timeline->tool == TIMELINE_TOOL_HAND) {
      container->dragScrollMove(event->pos()*container->zoom);
    } else if (event->buttons() & Qt::LeftButton) {
      if (gizmos == nullptr) {
        viewer->initiate_drag(olive::timeline::kImportBoth);
        dragging = false;
      } else {
        move_gizmos(event, false);
      }
    }
  } else {
    EffectGizmo* g = get_gizmo_from_mouse(event->pos().x(), event->pos().y());
    if (g != nullptr) {
      if (g->get_cursor() > -1) {
        setCursor(static_cast<enum Qt::CursorShape>(g->get_cursor()));
      }
    }
  }
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (dragging
      && gizmos != nullptr
      && event->button() == Qt::LeftButton
      && panel_timeline->tool != TIMELINE_TOOL_HAND) {
    move_gizmos(event, true);
  }
  dragging = false;
}

void ViewerWidget::wheelEvent(QWheelEvent *event) {
  container->parseWheelEvent(event);
}

void ViewerWidget::close_window() {
  window->hide();
}

void ViewerWidget::wait_until_render_is_paused()
{
  renderer.wait_until_paused();
}

void ViewerWidget::draw_waveform_func() {
  QPainter p(this);
  if (viewer->seq->using_workarea) {
    int in_x = getScreenPointFromFrame(waveform_zoom, viewer->seq->workarea_in) - waveform_scroll;
    int out_x = getScreenPointFromFrame(waveform_zoom, viewer->seq->workarea_out) - waveform_scroll;

    p.fillRect(QRect(in_x, 0, out_x - in_x, height()), QColor(255, 255, 255, 64));
    p.setPen(Qt::white);
    p.drawLine(in_x, 0, in_x, height());
    p.drawLine(out_x, 0, out_x, height());
  }
  QRect wr = rect();
  wr.setX(wr.x() - waveform_scroll);

  p.setPen(Qt::green);
  draw_waveform(waveform_clip, waveform_ms, waveform_clip->timeline_out(), &p, wr, waveform_scroll, width()+waveform_scroll, waveform_zoom);
  p.setPen(Qt::red);
  int playhead_x = getScreenPointFromFrame(waveform_zoom, viewer->seq->playhead) - waveform_scroll;
  p.drawLine(playhead_x, 0, playhead_x, height());
}

void ViewerWidget::draw_title_safe_area() {
  QOpenGLFunctions* func = context()->functions();

  pipeline_->bind();


  float ar = float(width()) / float(height());

  float horizontal_cross_size = 0.05f / ar;

  // Set matrix to 0.0 -> 1.0 on both axes
  QMatrix4x4 matrix;
  matrix.ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

  // adjust the horizontal center cross by the aspect ratio to appear "square"
  if (olive::CurrentConfig.use_custom_title_safe_ratio && olive::CurrentConfig.custom_title_safe_ratio > 0) {
    if (ar > olive::CurrentConfig.custom_title_safe_ratio) {
      matrix.translate(((ar - olive::CurrentConfig.custom_title_safe_ratio) / 2.0) / ar, 0.0f);
      matrix.scale(olive::CurrentConfig.custom_title_safe_ratio / ar, 1.0f);
    } else {
      matrix.translate(0.0f, (((olive::CurrentConfig.custom_title_safe_ratio - ar) / 2.0) / olive::CurrentConfig.custom_title_safe_ratio));
      matrix.scale(1.0f, ar / olive::CurrentConfig.custom_title_safe_ratio);
    }

    horizontal_cross_size *= ar/olive::CurrentConfig.custom_title_safe_ratio;
  }

  float adjusted_cross_x1 = 0.5f - horizontal_cross_size;
  float adjusted_cross_x2 = 0.5f + horizontal_cross_size;

  pipeline_->setUniformValue("mvp_matrix", matrix);
  pipeline_->setUniformValue("color_only", true);
  pipeline_->setUniformValue("color_only_color", QColor(192, 192, 192, 255));



  GLfloat vertices[] = {
    // action safe lines
    0.05f, 0.05f, 0.0f,
    0.95f, 0.05f, 0.0f,

    0.95f, 0.05f, 0.0f,
    0.95f, 0.95f, 0.0f,

    0.95f, 0.95f, 0.0f,
    0.05f, 0.95f, 0.0f,

    0.05f, 0.95f, 0.0f,
    0.05f, 0.05f, 0.0f,

    // title safe lines
    0.1f, 0.1f, 0.0f,
    0.9f, 0.1f, 0.0f,

    0.9f, 0.1f, 0.0f,
    0.9f, 0.9f, 0.0f,

    0.9f, 0.9f, 0.0f,
    0.1f, 0.9f, 0.0f,

    0.1f, 0.9f, 0.0f,
    0.1f, 0.1f, 0.0f,

    // side-center markers
    0.05f, 0.5f, 0.0f,
    0.125f, 0.5f, 0.0f,

    0.95f, 0.5f, 0.0f,
    0.875f, 0.5f, 0.0f,

    0.5f, 0.05f, 0.0f,
    0.5f, 0.125f, 0.0f,

    0.5f, 0.95f, 0.0f,
    0.5f, 0.875f, 0.0f,

    // horizontal center cross marker
    adjusted_cross_x1, 0.5f, 0.0f,
    adjusted_cross_x2, 0.5f, 0.0f,

    // vertical center cross marker
    0.5f, 0.45f, 0.0f,
    0.5f, 0.55f, 0.0f
  };

  vao_.bind();

  title_safe_area_buffer_.bind();
  title_safe_area_buffer_.write(0, vertices, kTitleActionSafeVertexSize * sizeof(GLfloat));

  GLuint vertex_location = pipeline_->attributeLocation("a_position");
  func->glEnableVertexAttribArray(vertex_location);
  func->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, 0);

  func->glDrawArrays(GL_LINES, 0, 28);

  pipeline_->setUniformValue("color_only", false);

  title_safe_area_buffer_.release();

  vao_.release();

  pipeline_->release();

}

void ViewerWidget::draw_gizmos() {
  QOpenGLFunctions* func = context()->functions();

  pipeline_->bind();

  double zoom_factor = container->zoom/(double(width())/double(viewer->seq->width));

  QMatrix4x4 matrix;
  matrix.ortho(0, viewer->seq->width, 0, viewer->seq->height, -1, 1);
  matrix.scale(zoom_factor, zoom_factor);
  matrix.translate(-(viewer->seq->width-(width()/container->zoom))*x_scroll,
                   -((viewer->seq->height-(height()/container->zoom))*(1.0-y_scroll)));

  // Set transformation matrix
  pipeline_->setUniformValue("mvp_matrix", matrix);

  // Set pipeline shader to draw full white
  pipeline_->setUniformValue("color_only", true);
  pipeline_->setUniformValue("color_only_color", QColor(255, 255, 255, 255));

  // Set up constants for gizmo sizes
  float size_diff = float(viewer->seq->width) / float(width());
  float dot_size = GIZMO_DOT_SIZE * size_diff;
  float target_size = GIZMO_TARGET_SIZE * size_diff;

  QVector<GLfloat> vertices;

  for (int j=0;j<gizmos->gizmo_count();j++) {

    EffectGizmo* g = gizmos->gizmo(j);

    switch (g->get_type()) {
    case GIZMO_TYPE_DOT:

      // Draw standard square dot

      vertices.append(g->screen_pos[0].x()-dot_size);
      vertices.append(g->screen_pos[0].y()-dot_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()+dot_size);
      vertices.append(g->screen_pos[0].y()-dot_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()+dot_size);
      vertices.append(g->screen_pos[0].y()+dot_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()-dot_size);
      vertices.append(g->screen_pos[0].y()+dot_size);
      vertices.append(0.0f);

      break;
    case GIZMO_TYPE_POLY:

      // Draw an arbitrary polygon with lines

      for (int k=1;k<g->get_point_count();k++) {

        vertices.append(g->screen_pos[k-1].x());
        vertices.append(g->screen_pos[k-1].y());
        vertices.append(0.0f);

        vertices.append(g->screen_pos[k].x());
        vertices.append(g->screen_pos[k].y());
        vertices.append(0.0f);

      }

      vertices.append(g->screen_pos[g->get_point_count()-1].x());
      vertices.append(g->screen_pos[g->get_point_count()-1].y());
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x());
      vertices.append(g->screen_pos[0].y());
      vertices.append(0.0f);

      break;
    case GIZMO_TYPE_TARGET:

      // Draw "target" gizmo (square with two lines through the middle)

      vertices.append(g->screen_pos[0].x()-target_size);
      vertices.append(g->screen_pos[0].y()-target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()+target_size);
      vertices.append(g->screen_pos[0].y()-target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()+target_size);
      vertices.append(g->screen_pos[0].y()-target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()+target_size);
      vertices.append(g->screen_pos[0].y()+target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()+target_size);
      vertices.append(g->screen_pos[0].y()+target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()-target_size);
      vertices.append(g->screen_pos[0].y()+target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()-target_size);
      vertices.append(g->screen_pos[0].y()+target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()-target_size);
      vertices.append(g->screen_pos[0].y()-target_size);
      vertices.append(0.0f);



      vertices.append(g->screen_pos[0].x()-target_size);
      vertices.append(g->screen_pos[0].y());
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x()+target_size);
      vertices.append(g->screen_pos[0].y());
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x());
      vertices.append(g->screen_pos[0].y()-target_size);
      vertices.append(0.0f);

      vertices.append(g->screen_pos[0].x());
      vertices.append(g->screen_pos[0].y()+target_size);
      vertices.append(0.0f);

      break;
    }
  }

  vao_.bind();

  // The gizmo buffer may have been destroyed or not created yet, ensure it's created here
  if (!gizmo_buffer_.isCreated() && !gizmo_buffer_.create()) {
    return;
  }

  gizmo_buffer_.bind();

  // Get the total byte size of the vertex array
  int gizmo_buffer_desired_size = vertices.size() * sizeof(GLfloat);

  // Determine if the gizmo count has changed, and if so reallocate the buffer
  if (gizmo_buffer_.size() != gizmo_buffer_desired_size) {
    gizmo_buffer_.allocate(vertices.constData(), gizmo_buffer_desired_size);
  } else {
    gizmo_buffer_.write(0, vertices.constData(), gizmo_buffer_desired_size);
  }


  GLuint vertex_location = pipeline_->attributeLocation("a_position");
  func->glEnableVertexAttribArray(vertex_location);
  func->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, 0);

  gizmo_buffer_.release();

  func->glDrawArrays(GL_LINES, 0, vertices.size() / 3);

  pipeline_->setUniformValue("color_only", false);

  vao_.release();

  pipeline_->release();

}

void ViewerWidget::paintGL() {
  if (waveform) {
    draw_waveform_func();
  } else {
    const GLuint tex = renderer.get_texture();
    QMutex* tex_lock = renderer.get_texture_mutex();

    tex_lock->lock();

    QOpenGLFunctions* f = context()->functions();

    makeCurrent();


    // clear to solid black
    f->glClearColor(0.0, 0.0, 0.0, 0.0);
    f->glClear(GL_COLOR_BUFFER_BIT);


    // draw texture from render thread

    f->glViewport(0, 0, width(), height());

    f->glBindTexture(GL_TEXTURE_2D, tex);

    olive::rendering::Blit(pipeline_.get(), true, get_matrix());

    f->glBindTexture(GL_TEXTURE_2D, 0);

    // draw title/action safe area
    if (olive::CurrentConfig.show_title_safe_area) {
      draw_title_safe_area();
    }

    gizmos = renderer.gizmos;
    if (gizmos != nullptr) {
      draw_gizmos();
    }

    if (window->isVisible()) {
      window->set_texture(tex, double(viewer->seq->width)/double(viewer->seq->height), tex_lock);
    }

    tex_lock->unlock();

    if (renderer.did_texture_fail() && !viewer->playing) {
      doneCurrent();
      renderer.start_render(context(), viewer->seq.get(), viewer->get_playback_speed());
    }
  }
}
