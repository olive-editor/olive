/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "multicamdisplay.h"

namespace olive {

#define super ViewerDisplayWidget

MulticamDisplay::MulticamDisplay(QWidget *parent) :
  super(parent),
  node_(nullptr),
  rows_(0),
  cols_(0)
{
}

void MulticamDisplay::OnPaint()
{
  super::OnPaint();

  if (node_) {
    QPainter p(paint_device());

    p.setPen(QPen(Qt::yellow, fontMetrics().height()/4));
    p.setBrush(Qt::NoBrush);

    int rows, cols;
    node_->GetRowsAndColumns(&rows, &cols);

    int multi = std::max(rows, cols);
    int cell_width = width() / multi;
    int cell_height = height() / multi;

    int col, row;
    node_->IndexToRowCols(node_->GetCurrentSource(), rows, cols, &row, &col);

    QRect r(cell_width * col, cell_height * row, cell_width, cell_height);
    p.drawRect(GenerateWorldTransform().mapRect(r));
  }
}

void MulticamDisplay::OnDestroy()
{
  shader_ = QVariant();
}

TexturePtr MulticamDisplay::LoadCustomTextureFromFrame(const QVariant &v)
{
  if (v.canConvert<QVector<TexturePtr> >()) {
    QVector<TexturePtr> tex = v.value<QVector<TexturePtr> >();

    TexturePtr main = renderer()->CreateTexture(this->GetViewportParams());

    int rows, cols;
    MultiCamNode::GetRowsAndColumns(tex.size(), &rows, &cols);

    if (shader_.isNull() || rows_ != rows || cols_ != cols) {
      if (!shader_.isNull()) {
        renderer()->DestroyNativeShader(shader_);
      }

      shader_ = renderer()->CreateNativeShader(ShaderCode(GenerateShaderCode(rows, cols)));

      rows_ = rows;
      cols_ = cols;
    }

    ShaderJob job;

    for (int i=0; i<tex.size(); i++) {
      int c, r;
      MultiCamNode::IndexToRowCols(i, rows, cols, &r, &c);
      job.Insert(QStringLiteral("tex_%1_%2").arg(QString::number(r), QString::number(c)), NodeValue(NodeValue::kTexture, tex.at(i)));
    }

    renderer()->BlitToTexture(shader_, job, main.get());

    return main;
  } else {
    return super::LoadCustomTextureFromFrame(v);
  }
}

QString dblToGlsl(double d)
{
  return QString::number(d, 'f');
}

QString MulticamDisplay::GenerateShaderCode(int rows, int cols)
{
  int multiplier = std::max(cols, rows);

  QStringList shader;

  shader.append(QStringLiteral("in vec2 ove_texcoord;"));
  shader.append(QStringLiteral("out vec4 frag_color;"));

  for (int x=0;x<cols;x++) {
    for (int y=0;y<rows;y++) {
      shader.append(QStringLiteral("uniform sampler2D tex_%1_%2;").arg(QString::number(y), QString::number(x)));
      shader.append(QStringLiteral("uniform bool tex_%1_%2_enabled;").arg(QString::number(y), QString::number(x)));
    }
  }

  shader.append(QStringLiteral("void main() {"));

  for (int x=0;x<cols;x++) {
    if (x > 0) {
      shader.append(QStringLiteral("  else"));
    }
    if (x == cols-1) {
      shader.append(QStringLiteral("  {"));
    } else {
      shader.append(QStringLiteral("  if (ove_texcoord.x < %1) {").arg(dblToGlsl(double(x+1)/double(multiplier))));
    }

    for (int y=0;y<rows;y++) {
      if (y > 0) {
        shader.append(QStringLiteral("    else"));
      }
      if (y == rows-1) {
        shader.append(QStringLiteral("    {"));
      } else {
        shader.append(QStringLiteral("    if (ove_texcoord.y < %1) {").arg(dblToGlsl(double(y+1)/double(multiplier))));
      }
      QString input = QStringLiteral("tex_%1_%2").arg(QString::number(y), QString::number(x));
      shader.append(QStringLiteral("      vec2 coord = vec2((ove_texcoord.x+%1)*%2, (ove_texcoord.y+%3)*%4);").arg(
                      dblToGlsl( - double(x)/double(multiplier)),
                      dblToGlsl(multiplier),
                      dblToGlsl( - double(y)/double(multiplier)),
                      dblToGlsl(multiplier)
                      ));
      shader.append(QStringLiteral("      if (%1_enabled && coord.x >= 0.0 && coord.x < 1.0 && coord.y >= 0.0 && coord.y < 1.0) {").arg(input));
      shader.append(QStringLiteral("        frag_color = texture(%1, coord);").arg(input));
      shader.append(QStringLiteral("      } else {"));
      shader.append(QStringLiteral("        discard;"));
      shader.append(QStringLiteral("      }"));
      shader.append(QStringLiteral("    }"));
    }

    shader.append(QStringLiteral("  }"));
  }

  shader.append(QStringLiteral("}"));

  return shader.join('\n');
}

void MulticamDisplay::SetMulticamNode(MultiCamNode *n)
{
  node_ = n;
}

}
