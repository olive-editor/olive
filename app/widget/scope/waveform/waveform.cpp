
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
#include <QVector2D>
#include <QVector3D>

#include "common/qtutils.h"
#include "node/node.h"

OLIVE_NAMESPACE_ENTER

WaveformScope::WaveformScope(QWidget* parent) :
  ScopeBase(parent)
{
}

WaveformScope::~WaveformScope()
{
  OnDestroy();
}

ShaderCode WaveformScope::GenerateShaderCode()
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/rgbwaveform.frag"),
                    FileFunctions::ReadFileAsString(":/shaders/rgbwaveform.vert"));
}

void WaveformScope::DrawScope(Renderer::TexturePtr managed_tex, QVariant pipeline)
{
  float waveform_scale = 0.80f;

  // Draw waveform through shader
  ShaderJob job;

  // Set viewport size
  job.InsertValue(QStringLiteral("viewport"),
                  ShaderValue(QVector2D(width(), height()), NodeParam::kVec2));

  // Set luma coefficients
  float luma_coeffs[3] = {0.0f, 0.0f, 0.0f};
  color_manager()->GetDefaultLumaCoefs(luma_coeffs);
  job.InsertValue(QStringLiteral("luma_coeffs"),
                  ShaderValue(QVector3D(luma_coeffs[0], luma_coeffs[1], luma_coeffs[2]), NodeParam::kVec3));


  // Scale of the waveform relative to the viewport surface.
  job.InsertValue(QStringLiteral("waveform_scale"),
                  ShaderValue(waveform_scale, NodeParam::kFloat));

  // Insert source texture
  job.InsertValue(QStringLiteral("ove_maintex"),
                  ShaderValue(QVariant::fromValue(managed_tex), NodeParam::kTexture));

  renderer()->Blit(pipeline, job, VideoParams(width(), height(), PixelFormat::PIX_FMT_RGBA16F));

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
