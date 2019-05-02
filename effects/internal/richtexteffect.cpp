#include "richtexteffect.h"

#include <QTextDocument>
#include <QtMath>

#include "timeline/clip.h"
#include "ui/blur.h"

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

  EffectRow* shadow_row = new EffectRow(this, tr("Shadow"));
  shadow_bool = new BoolField(shadow_row, "shadow");
  shadow_bool->SetColumnSpan(2);

  EffectRow* shadow_color_row = new EffectRow(this, tr("Shadow Color"));
  shadow_color = new ColorField(shadow_color_row, "shadowcolor");
  shadow_color->SetColumnSpan(2);

  EffectRow* shadow_angle_row = new EffectRow(this, tr("Shadow Angle"));
  shadow_angle = new DoubleField(shadow_angle_row, "shadowangle");
  shadow_angle->SetColumnSpan(2);

  EffectRow* shadow_distance_row = new EffectRow(this, tr("Shadow Distance"));
  shadow_distance = new DoubleField(shadow_distance_row, "shadowdistance");
  shadow_distance->SetColumnSpan(2);
  shadow_distance->SetMinimum(0);

  EffectRow* shadow_softness_row = new EffectRow(this, tr("Shadow Softness"));
  shadow_softness = new DoubleField(shadow_softness_row, "shadowsoftness");
  shadow_softness->SetColumnSpan(2);
  shadow_softness->SetMinimum(0);

  EffectRow* shadow_opacity_row = new EffectRow(this, tr("Shadow Opacity"));
  shadow_opacity = new DoubleField(shadow_opacity_row, "shadowopacity");
  shadow_opacity->SetColumnSpan(2);
  shadow_opacity->SetMinimum(0);
  shadow_opacity->SetMaximum(100);

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

  double scroll_progress = 0;

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

      int doc_width = qRound(td.size().width());
      translate_x += qRound(-doc_width + (img.width() + doc_width) * scroll_progress);
    }

  } else if (auto_scroll_dir == SCROLL_UP || auto_scroll_dir == SCROLL_DOWN) {

    // Auto-scroll bottom to top or top to bottom

    if (auto_scroll_dir == SCROLL_UP) {
      scroll_progress = 1.0 - scroll_progress;
    }

    translate_y += qRound(-doc_height + (img.height() + doc_height)*scroll_progress);

  }

  QRect clip_rect = img.rect();
  clip_rect.translate(-translate_x, -translate_y);
  p.translate(translate_x, translate_y);

  img.fill(Qt::transparent);

  // draw software shadow
  if (shadow_bool->GetBoolAt(timecode)) {

    // calculate offset using distance and angle
    double angle = shadow_angle->GetDoubleAt(timecode) * M_PI / 180.0;
    double distance = qFloor(shadow_distance->GetDoubleAt(timecode));
    int shadow_x_offset = qRound(qCos(angle) * distance);
    int shadow_y_offset = qRound(qSin(angle) * distance);

    p.translate(shadow_x_offset, shadow_y_offset);
    clip_rect.translate(-shadow_x_offset, -shadow_y_offset);

    td.drawContents(&p, clip_rect);

    int blurSoftness = qFloor(shadow_softness->GetDoubleAt(timecode));
    if (blurSoftness > 0) {
      olive::ui::blur(img, img.rect(), blurSoftness, true);
    }

    p.setCompositionMode(QPainter::CompositionMode_SourceIn);

    p.fillRect(clip_rect, shadow_color->GetColorAt(timecode));

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    p.translate(-shadow_x_offset, -shadow_y_offset);
    clip_rect.translate(shadow_x_offset, shadow_y_offset);
  }

  td.drawContents(&p, clip_rect);

  p.end();
}

bool RichTextEffect::AlwaysUpdate()
{
  return autoscroll->GetValueAt(autoscroll->Now()).toInt() != SCROLL_OFF;
}
