
/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "waveform.h"

#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGuiApplication>

#include "common/qtutils.h"
#include "node/node.h"
#include "render/backend/opengl/openglrenderfunctions.h"

OLIVE_NAMESPACE_ENTER

WaveformScope::WaveformScope(QWidget* parent) :
  ScopeBase(parent)
{
  SetupControlUI();
}

void WaveformScope::SetupControlUI()
{
  //control_ui_ = new QWidget();
  control_ui_->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout* toolbar_layout = new QHBoxLayout(control_ui_);
  toolbar_layout->setMargin(0);

  swizzle_.reserve(4);
  swizzle_ = {true, true, true, false};

  // check boxes for channels in waveform
  luma_select_ = new QCheckBox(tr("Luma"), this);
  toolbar_layout->addWidget(luma_select_);

  red_select_ = new QCheckBox(tr("R"), this);
  red_select_->setChecked(true);
  toolbar_layout->addWidget(red_select_);

  green_select_ = new QCheckBox(tr("G"), this);
  green_select_->setChecked(true);
  toolbar_layout->addWidget(green_select_);

  blue_select_ = new QCheckBox(tr("B"), this);
  blue_select_->setChecked(true);
  toolbar_layout->addWidget(blue_select_);

  connect(luma_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(3); });
  connect(red_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(0); });
  connect(green_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(1); });
  connect(blue_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(2); });
}

void WaveformScope::SortSwizzleData(int checkbox) {
  // If Ctrl is pressed when selecting a channel, display only the
  // the channel selected
  if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
    for (int i = 0; i < 4; i++) {
      swizzle_[i] = (i == checkbox ? true : false);
    }
    red_select_->setChecked(checkbox == 0 ? true : false);
    green_select_->setChecked(checkbox == 1 ? true : false);
    blue_select_->setChecked(checkbox == 2 ? true : false);
    luma_select_->setChecked(checkbox == 3 ? true : false);
  } else {
    swizzle_[checkbox] = !swizzle_[checkbox];
  }

  update();
}

OpenGLShaderPtr WaveformScope::CreateShader()
{
  OpenGLShaderPtr pipeline = OpenGLShader::Create();

  pipeline->create();
  pipeline->addShaderFromSourceCode(QOpenGLShader::Vertex,
    Node::ReadFileAsString(":/shaders/rgbwaveform.vert"));
  pipeline->addShaderFromSourceCode(QOpenGLShader::Fragment,
    Node::ReadFileAsString(":/shaders/rgbwaveform.frag"));
  pipeline->link();

  return pipeline;
}

void WaveformScope::DrawScope()
{
  float waveform_scale = 0.80f;

  // Draw waveform through shader
  pipeline()->bind();
  pipeline()->setUniformValue("ove_resolution", managed_tex().width(), managed_tex().height());
  pipeline()->setUniformValue("ove_viewport", width(), height());
  GLfloat luma[3] = {0.0, 0.0, 0.0};
  color_manager()->GetDefaultLumaCoefs(luma);
  pipeline()->setUniformValue("luma_coeffs", luma[0], luma[1], luma[2]);

  // Scale of the waveform relative to the viewport surface.
  pipeline()->setUniformValue("waveform_scale", waveform_scale);

  // channel swizzle
  pipeline()->setUniformValue("channel_swizzle", swizzle_[0], swizzle_[1], swizzle_[2], swizzle_[3]);

  pipeline()->release();

  managed_tex().Bind();

  OpenGLRenderFunctions::Blit(pipeline());

  managed_tex().Release();

  float waveform_dim_x = ceil((width() - 1.0) * waveform_scale);
  float waveform_dim_y = ceil((height() - 1.0) * waveform_scale);
  float waveform_start_dim_x =
    ((width() - 1.0) - waveform_dim_x) / 2.0f;
  float waveform_start_dim_y =
    ((height() - 1.0) - waveform_dim_y) / 2.0f;
  float waveform_end_dim_x = (width() - 1.0) - waveform_start_dim_x;

  // Draw line overlays
  QPainter p(this);
  QFont font;
  font.setPixelSize(10);
  QFontMetrics font_metrics = QFontMetrics(font);
  QString label;
  float ire_increment = 0.1f;
  int ire_steps = qRound(1.0 / ire_increment);
  QVector<QLine> ire_lines(ire_steps + 1);
  int font_x_offset = 0;
  int font_y_offset = font_metrics.capHeight() / 2.0f;

  p.setCompositionMode(QPainter::CompositionMode_Plus);

  p.setPen(QColor(0.0, 0.6 * 255.0, 0.0));
  p.setFont(font);

  for (int i=0; i <= ire_steps; i++) {
    ire_lines[i].setLine(
      waveform_start_dim_x,
      (waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y,
      waveform_end_dim_x,
      (waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y);
      label = QString::number(1.0 - (i * ire_increment), 'f', 1);
      font_x_offset = QFontMetricsWidth(font_metrics, label) + 4;

      p.drawText(
        waveform_start_dim_x - font_x_offset,
        (waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y + font_y_offset,
        label);
  }
  p.drawLines(ire_lines);
}

OLIVE_NAMESPACE_EXIT
