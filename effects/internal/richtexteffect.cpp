#include "richtexteffect.h"

#include <QTextDocument>

RichTextEffect::RichTextEffect(Clip *c, const EffectMeta *em) :
  Effect(c, em)
{
  SetFlags(Effect::SuperimposeFlag);

  EffectRow* text_row = new EffectRow(this, tr("Text"));
  text_val = new StringField(text_row, "text");

  EffectRow* padding_row = new EffectRow(this, tr("Padding"));
  padding_field = new DoubleField(padding_row, "padding");
  padding_field->SetColumnSpan(2);

  EffectRow* position_row = new EffectRow(this, tr("Position"));
  position_x = new DoubleField(position_row, "posx");
  position_y = new DoubleField(position_row, "posy");

  EffectRow* vertical_align_row = new EffectRow(this, tr("Vertical Align:"));
  vertical_align = new ComboField(vertical_align_row, "valign");
  vertical_align->AddItem(tr("Top"), Qt::AlignTop);
  vertical_align->AddItem(tr("Center"), Qt::AlignCenter);
  vertical_align->AddItem(tr("Bottom"), Qt::AlignBottom);
  vertical_align->SetValueAt(0, Qt::AlignCenter);

  // Create default text
  text_val->SetValueAt(0, "<html>"
                            "<body style=\"color: #ffffff; font-size: 36pt;\">"
                              "<center>Sample Text</center>"
                            "</body>"
                          "</html>");
}

void RichTextEffect::redraw(double timecode)
{
  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  int width = img.width();
  int height = img.height();

  img.fill(Qt::transparent);

  int padding = qRound(padding_field->GetDoubleAt(timecode));

  width -= 2 * padding;
  height -= 2 * padding;

  QTextDocument td;
  td.setHtml(text_val->GetStringAt(timecode));
  td.setTextWidth(width);

  int translate_x = qRound(position_x->GetDoubleAt(timecode) + padding);
  int translate_y = qRound(position_y->GetDoubleAt(timecode) + padding);

  if (vertical_align->GetValueAt(timecode).toInt() == Qt::AlignCenter) {
    translate_y += height / 2 - td.size().height() / 2;
  } else if (vertical_align->GetValueAt(timecode).toInt() == Qt::AlignBottom) {
    translate_y += height - td.size().height();
  }

  QRect clip_rect = img.rect();
  clip_rect.translate(-translate_x, -translate_y);
  p.translate(translate_x, translate_y);


  td.drawContents(&p, clip_rect);
}
