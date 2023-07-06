#include "multicamnode.h"

#include "node/project/sequence/sequence.h"

namespace olive {

#define super Node

const QString MultiCamNode::kCurrentInput = QStringLiteral("current_in");
const QString MultiCamNode::kSourcesInput = QStringLiteral("sources_in");
const QString MultiCamNode::kSequenceInput = QStringLiteral("sequence_in");
const QString MultiCamNode::kSequenceTypeInput = QStringLiteral("sequence_type_in");

MultiCamNode::MultiCamNode()
{
  AddInput(kCurrentInput, TYPE_COMBO, kInputFlagStatic);

  AddInput(kSourcesInput, kInputFlagNotKeyframable | kInputFlagArray);
  SetInputProperty(kSourcesInput, QStringLiteral("arraystart"), 1);

  AddInput(kSequenceInput, kInputFlagNotKeyframable);
  AddInput(kSequenceTypeInput, TYPE_COMBO, kInputFlagStatic | kInputFlagHidden);

  sequence_ = nullptr;
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

QString dblToGlsl(double d)
{
  return QString::number(d, 'f');
}

QString GenerateShaderCode(int rows, int cols)
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

ShaderCode MultiCamNode::GetShaderCode(const QString &id)
{
  auto l = id.split(':');
  int rows = l.at(0).toInt();
  int cols = l.at(1).toInt();

  return ShaderCode(GenerateShaderCode(rows, cols));
}

value_t MultiCamNode::Value(const ValueParams &p) const
{
  if (p.output() == QStringLiteral("all")) {
    // Switcher mode: output a collage of all sources
    int sources = GetSourceCount();
    int rows, cols;
    MultiCamNode::GetRowsAndColumns(sources, &rows, &cols);

    ShaderJob job;
    job.SetShaderID(QStringLiteral("%1:%2").arg(QString::number(rows), QString::number(cols)));
    job.set_function(GetShaderCode);

    for (int i=0; i<sources; i++) {
      int c, r;
      MultiCamNode::IndexToRowCols(i, rows, cols, &r, &c);

      NodeOutput o = GetSourceNode(i);
      if (o.IsValid()) {
        value_t v = GetFakeConnectedValue(p, o, kSourcesInput, i);
        job.Insert(QStringLiteral("tex_%1_%2").arg(QString::number(r), QString::number(c)), v);
      }
    }

    return value_t(Texture::Job(p.vparams(), job));
  } else {
    // Default behavior: output currently selected source
    int current = GetInputValue(p, kCurrentInput).toInt();

    NodeOutput o = GetSourceNode(current);
    if (o.IsValid()) {
      return GetFakeConnectedValue(p, o, kSourcesInput, current);
    }
  }

  return value_t();
}

void MultiCamNode::IndexToRowCols(int index, int total_rows, int total_cols, int *row, int *col)
{
  Q_UNUSED(total_rows)

  *col = index%total_cols;
  *row = index/total_cols;
}

void MultiCamNode::InputConnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kSequenceInput) {
    if (Sequence *s = dynamic_cast<Sequence*>(output.node())) {
      SetInputFlag(kSequenceTypeInput, kInputFlagHidden, false);
      sequence_ = s;
    }
  }
}

void MultiCamNode::InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kSequenceInput) {
    SetInputFlag(kSequenceTypeInput, kInputFlagHidden, true);
    sequence_ = nullptr;
  }
}

NodeOutput MultiCamNode::GetSourceNode(int source) const
{
  if (sequence_) {
    return NodeOutput(GetTrackList()->GetTrackAt(source));
  } else {
    return GetConnectedOutput2(kSourcesInput, source);
  }
}

TrackList *MultiCamNode::GetTrackList() const
{
  return sequence_->track_list(static_cast<Track::Type>(GetStandardValue(kSequenceTypeInput).toInt()));
}

void MultiCamNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kCurrentInput, tr("Current"));
  SetInputName(kSourcesInput, tr("Sources"));
  SetInputName(kSequenceInput, tr("Sequence"));
  SetInputName(kSequenceTypeInput, tr("Sequence Type"));
  SetComboBoxStrings(kSequenceTypeInput, {tr("Video"), tr("Audio")});

  QStringList names;
  int name_count = GetSourceCount();
  names.reserve(name_count);
  for (int i=0; i<name_count; i++) {
    QString src_name;
    NodeOutput o = GetSourceNode(i);
    if (o.IsValid()) {
      src_name = o.node()->Name();
    }
    names.append(tr("%1: %2").arg(QString::number(i+1), src_name));
  }
  SetComboBoxStrings(kCurrentInput, names);
}

int MultiCamNode::GetSourceCount() const
{
  if (sequence_) {
    return GetTrackList()->GetTrackCount();
  } else {
    return InputArraySize(kSourcesInput);
  }
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
