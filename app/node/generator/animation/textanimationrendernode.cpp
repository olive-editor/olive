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

#include "textanimationrendernode.h"

#include <QGuiApplication>
#include <QVector2D>
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include <QTextCursor>

#include "common/cpuoptimize.h"
#include "common/html.h"
#include "node/generator/animation/textanimationxmlparser.h"

namespace olive {

const QString TextAnimationRenderNode::kPositionInput = QStringLiteral("pos_in");
const QString TextAnimationRenderNode::kRichTextInput = QStringLiteral("rich_text_in");
const QString TextAnimationRenderNode::kAnimatorsInput = QStringLiteral("animators_in");
const QString TextAnimationRenderNode::kLineHeightInput = QStringLiteral("line_height_in");


#define super Node

TextAnimationRenderNode::TextAnimationRenderNode() :
  position_handle_(nullptr)
{
  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(200, 200));
  AddInput( kRichTextInput, NodeValue::kText,
            QStringLiteral("<p style='font-size: 72pt; color: white;'>%1</p>").arg(tr("Sample Text")),
            InputFlags(kInputFlagNotKeyframable));
  AddInput( kAnimatorsInput, NodeValue::kText,
            QStringLiteral("<!-- Don't write here. Attach a TextAnimation node -->"),
            InputFlags(kInputFlagNotKeyframable));
  AddInput( kLineHeightInput, NodeValue::kInt, 50, InputFlags(kInputFlagNotKeyframable));

  position_handle_ = AddDraggableGizmo<PointGizmo>({NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0),
                                                    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1)});

  position_handle_->SetShape( PointGizmo::kAnchorPoint);
  position_handle_->SetDragValueBehavior(PointGizmo::kAbsolute);

  SetEffectInput(kRichTextInput);

  engine_ = new TextAnimationEngine();
}

QString TextAnimationRenderNode::Name() const
{
  return tr("Text Animation Render");
}

QString TextAnimationRenderNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.TextAnimationRenderNode");
}

QVector<Node::CategoryID> TextAnimationRenderNode::Category() const
{
  return {kCategoryGenerator};
}

QString TextAnimationRenderNode::Description() const
{
  return tr("Takes a rich text and a set of animation descriptors to produce the text animation");
}

void TextAnimationRenderNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kPositionInput, tr("Position"));
  SetInputName(kRichTextInput, tr("Text"));
  SetInputName(kAnimatorsInput, tr("Animators"));
  SetInputName(kLineHeightInput, tr("Line Height"));
}

GenerateJob TextAnimationRenderNode::GetGenerateJob(const NodeValueRow &value) const
{
  GenerateJob job;

  job.Insert(value);
  job.SetRequestedFormat(VideoParams::kFormatUnsigned8);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  return job;
}

void TextAnimationRenderNode::Value(const NodeValueRow &value, const NodeGlobals & /*globals*/, NodeValueTable *table) const
{
  GenerateJob job = GetGenerateJob(value);

  table->Push(NodeValue::kTexture, job, this);
}


void TextAnimationRenderNode::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  QImage img(reinterpret_cast<uchar*>(frame->data()), frame->width(), frame->height(),
             frame->linesize_bytes(), QImage::Format_RGBA8888_Premultiplied);
  img.fill(Qt::transparent);

  // 96 DPI in DPM (96 / 2.54 * 100)
  const int dpm = 3780;
  img.setDotsPerMeterX(dpm);
  img.setDotsPerMeterY(dpm);

  QTextDocument text_doc;
  text_doc.setDefaultFont(QFont("Arial", 50));

  QString html = job.Get(kRichTextInput).toString();
  Html::HtmlToDoc(&text_doc, html);
  QTextCursor cursor( &text_doc);

  // aspect ratio
  double par = frame->video_params().pixel_aspect_ratio().toDouble();
  QPointF scale_factor = QPointF(1.0 / frame->video_params().divider() / par, 1.0 / frame->video_params().divider());


  QPainter p(&img);
  p.scale( scale_factor.x(), scale_factor.y());

  QVector2D pos = job.Get(kPositionInput).toVec2();

  p.translate(pos.x(), pos.y());

  p.setBrush(Qt::NoBrush);
  p.setPen(Qt::white);

  p.setRenderHints( QPainter::Antialiasing |
                    QPainter::SmoothPixmapTransform);

  int textSize = text_doc.characterCount();
  cursor.movePosition( QTextCursor::Start);
  cursor.movePosition( QTextCursor::Right);

  // 'kAnimatorsInput' holds a list of XML tags without main opening and closing tags.
  // Prepend and append XML start and closing tags.
  QString animators_xml = QStringLiteral("<TR>\n") + job.Get(kAnimatorsInput).toString() +
                          QStringLiteral("</TR>");

  QList<TextAnimation::Descriptor> animators = TextAnimationXmlParser::Parse( animators_xml);

  engine_->SetTextSize( textSize);
  engine_->SetAnimators( & animators);
  engine_->Calulate();

  int posx = 0;
  int line = 0;
  int line_height = job.Get(kLineHeightInput).toInt();


  const QVector<double> & horiz_offsets = engine_->HorizontalOffset();
  const QVector<double> & vert_offsets = engine_->VerticalOffset();
  const QVector<double> & rotations = engine_->Rotation();
  const QVector<double> & spacings = engine_->Spacing();

  // parse the whole text document and print every character
  for (int i=0; i < textSize; i++) {
      QChar ch = text_doc.characterAt(i);

      p.save();
      p.setFont( cursor.charFormat().font());
      p.setPen( cursor.charFormat().foreground().color());
      p.setBrush( cursor.charFormat().foreground());
      p.translate( QPointF(posx + horiz_offsets[i],
                           line*line_height + vert_offsets[i]));
      p.rotate( rotations[i]);
      p.drawText( QPointF(0,0), ch);
      //p.resetTransform();
      p.restore();

      posx += QFontMetrics( cursor.charFormat().font()).horizontalAdvance(text_doc.characterAt(i)) + (int)(spacings[i]);
      cursor.movePosition( QTextCursor::Right);

      if ((ch == QChar::LineSeparator) ||
          (ch == QChar::ParagraphSeparator) )
      {
        line++;
        posx = 0;
      }
  }
}


void TextAnimationRenderNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers & /*modifiers*/)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  gizmo->GetDraggers()[0].Drag( x);
  gizmo->GetDraggers()[1].Drag( y);
}

void TextAnimationRenderNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals & /*globals*/)
{
  QVector2D position_vec = row[kPositionInput].toVec2();
  QPointF position_pt =  QPointF(position_vec.x(), position_vec.y());

  // Create handles
  position_handle_->SetPoint(position_pt);
}

}  // olive
