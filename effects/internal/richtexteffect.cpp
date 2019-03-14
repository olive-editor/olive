#include "richtexteffect.h"

#include <QTextDocument>

RichTextEffect::RichTextEffect(Clip *c, const EffectMeta *em) :
  Effect(c, em)
{
  EffectRow* text_row = new EffectRow(this, tr("Text"));
  text_val = new StringField(text_row, "text");
}

void RichTextEffect::redraw(double timecode)
{
  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  int width = img.width();
  int height = img.height();

  img.fill(Qt::transparent);

  QTextDocument td;
  td.setHtml(text_val->GetStringAt(timecode));
  td.drawContents(&p);
}
