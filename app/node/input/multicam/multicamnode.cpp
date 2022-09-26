#include "multicamnode.h"

namespace olive {

#define super Node

const QString MultiCamNode::kCurrentInput = QStringLiteral("current_in");
const QString MultiCamNode::kSourcesInput = QStringLiteral("sources_in");

MultiCamNode::MultiCamNode()
{
  AddInput(kCurrentInput, NodeValue::kInt, InputFlags(kInputFlagStatic));

  // Make current index start at 1 instead of 0
  SetInputProperty(kCurrentInput, QStringLiteral("offset"), 1);
  SetInputProperty(kCurrentInput, QStringLiteral("min"), 0);

  AddInput(kSourcesInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable | kInputFlagArray));
  SetInputProperty(kSourcesInput, QStringLiteral("arraystart"), 1);

  monitor_ = false;
}

QString MultiCamNode::Name() const
{
  return tr("Multi-Cam");
}

QString MultiCamNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.multicam");
}

QVector<Node::CategoryID> MultiCamNode::Category() const
{
  return {kCategoryTimeline};
}

QString MultiCamNode::Description() const
{
  return tr("Allows easy switching between multiple sources.");
}

Node::ActiveElements MultiCamNode::GetActiveElementsAtTime(const QString &input, const TimeRange &r) const
{
  if (input == kSourcesInput && !monitor_) {
    Node::ActiveElements a;
    a.add(GetStandardValue(kCurrentInput).toInt());
    return a;
  } else {
    return super::GetActiveElementsAtTime(input, r);
  }
}

QString dblToGlsl(double d)
{
  return QString::number(d, 'f');
}

ShaderCode MultiCamNode::GetShaderCode(const ShaderRequest &id) const
{
  QStringList pieces = id.id.split(',');
  int rows = pieces.at(0).toInt();
  int cols = pieces.at(1).toInt();
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

  return ShaderCode(shader.join('\n'));
}

void MultiCamNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (!monitor_) {
    NodeValueArray arr = value[kSourcesInput].toArray();
    if (!arr.empty()) {
      table->Push(arr.begin()->second);
    }
  } else {
    NodeValueArray arr = value[kSourcesInput].toArray();

    int rows, cols;
    GetRowsAndColumns(arr.size(), &rows, &cols);

    ShaderJob job;

    job.SetShaderID(QStringLiteral("%1,%2").arg(QString::number(rows), QString::number(cols)));

    for (size_t i=0; i<arr.size(); i++) {
      size_t c = i%cols;
      size_t r = i/cols;
      job.Insert(QStringLiteral("tex_%1_%2").arg(QString::number(r), QString::number(c)), arr[i]);
    }

    table->Push(NodeValue::kTexture, Texture::Job(globals.vparams(), job), this);
  }
}

void MultiCamNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kCurrentInput, tr("Current"));
  SetInputName(kSourcesInput, tr("Sources"));
}

void MultiCamNode::GetRowsAndColumns(int sources, int *rows_in, int *cols_in)
{
  int &rows = *rows_in;
  int &cols = *cols_in;

  rows = 1;
  cols = 1;
  while (rows*cols < sources) {
    if (rows < cols) {
      rows++;
    } else {
      cols++;
    }
  }
}

}
