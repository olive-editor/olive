#include "richtexteffect.h"

#include <QTextDocument>

#include "timeline/clip.h"

enum AutoscrollDirection {
  SCROLL_OFF,
  SCROLL_UP,
  SCROLL_DOWN,
  SCROLL_LEFT,
  SCROLL_RIGHT,
};

RichTextEffect::RichTextEffect(Clip *c, const EffectMeta *em) :
  Effect(c, em)
{
  SetFlags(Effect::SuperimposeFlag);

  EffectRow* text_row = new EffectRow(this, tr("Text"));
  text_val = new StringField(text_row, "text");
  text_val->SetColumnSpan(2);

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
  vertical_align->SetColumnSpan(2);

  EffectRow* autoscroll_row = new EffectRow(this, tr("Auto-Scroll"));
  autoscroll = new ComboField(autoscroll_row, "autoscroll");
  autoscroll->AddItem(tr("Off"), SCROLL_OFF);
  autoscroll->AddItem(tr("Up"), SCROLL_UP);
  autoscroll->AddItem(tr("Down"), SCROLL_DOWN);
  autoscroll->AddItem(tr("Left"), SCROLL_LEFT);
  autoscroll->AddItem(tr("Right"), SCROLL_RIGHT);
  autoscroll->SetColumnSpan(2);

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

  int doc_height = qRound(td.size().height());

  AutoscrollDirection auto_scroll_dir = static_cast<AutoscrollDirection>(autoscroll->GetValueAt(timecode).toInt());

  double scroll_progress;

  if (auto_scroll_dir != SCROLL_OFF) {
    double clip_length_secs = double(parent_clip->length()) / parent_clip->media_frame_rate();
    scroll_progress = (timecode - double(parent_clip->clip_in()) / parent_clip->media_frame_rate()) / clip_length_secs;
  }

  if (auto_scroll_dir == SCROLL_OFF || auto_scroll_dir == SCROLL_LEFT || auto_scroll_dir == SCROLL_RIGHT) {

    // If we're not auto-scrolling the vertical direction, respect the vertical alignment
    if (vertical_align->GetValueAt(timecode).toInt() == Qt::AlignCenter) {
      translate_y += height / 2 - doc_height / 2;
    } else if (vertical_align->GetValueAt(timecode).toInt() == Qt::AlignBottom) {
      translate_y += height - doc_height;
    }

    // Check if we are autoscrolling
    if (auto_scroll_dir != SCROLL_OFF) {

      if (auto_scroll_dir == SCROLL_LEFT) {
        scroll_progress = 1.0 - scroll_progress;
      }

      int doc_width = td.size().width();
      translate_x += qRound(-doc_width + (img.width() + doc_width) * scroll_progress);
    }

  } else if (auto_scroll_dir == SCROLL_UP || auto_scroll_dir == SCROLL_DOWN) {

    // Auto-scroll bottom to top or top to bottom

    if (auto_scroll_dir == SCROLL_UP) {
      scroll_progress = 1.0 - scroll_progress;
    }

    translate_y += qRound(-doc_height + (height + doc_height)*scroll_progress);

  }

  QRect clip_rect = img.rect();
  clip_rect.translate(-translate_x, -translate_y);
  p.translate(translate_x, translate_y);


  td.drawContents(&p, clip_rect);
}

bool RichTextEffect::AlwaysUpdate()
{
  return autoscroll->GetValueAt(autoscroll->Now()).toInt() != SCROLL_OFF;
}
