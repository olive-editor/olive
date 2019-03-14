#include "richtexteffect.h"

RichTextEffect::RichTextEffect(Clip *c, const EffectMeta *em)
{

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
