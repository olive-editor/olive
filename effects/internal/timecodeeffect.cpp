#include "timecodeeffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QOpenGLTexture>
#include <QTextEdit>
#include <QPainter>
#include <QPushButton>
#include <QColorDialog>
#include <QFontDatabase>
#include <QComboBox>
#include <QWidget>
#include <QtMath>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "panels/viewer.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "ui/fontcombobox.h"
#include "io/config.h"
#include "playback/playback.h"

TimecodeEffect::TimecodeEffect(Clip *c, const EffectMeta* em) :
    Effect(c, em)
{
    enable_always_update = true;
	enable_superimpose = true;

    EffectRow* tc_row = add_row("Timecode:");
    tc_select = tc_row->add_field(EFFECT_FIELD_COMBO);
    tc_select->add_combo_item("Sequence", true);
    tc_select->add_combo_item("Media", false);
    tc_select->set_combo_index(0);
    tc_select->id = "tc_selector";

    scale_val = add_row("Scale:")->add_field(EFFECT_FIELD_DOUBLE, 2);
    scale_val->set_double_minimum_value(1);
    scale_val->set_double_default_value(100);
    scale_val->set_double_maximum_value(1000);
    scale_val->id = "scale";

    color_val = add_row("Color:")->add_field(EFFECT_FIELD_COLOR, 2);
    color_val->set_color_value(Qt::white);
    color_val->id = "color";

    color_bg_val = add_row("Background Color:")->add_field(EFFECT_FIELD_COLOR, 2);
    color_bg_val->set_color_value(Qt::black);
    color_bg_val->id = "bgcolor";

    bg_alpha = add_row("Background Opacity:")->add_field(EFFECT_FIELD_DOUBLE, 2);
    bg_alpha->set_double_minimum_value(0);
    bg_alpha->set_double_maximum_value(100);
    bg_alpha->set_double_default_value(50);

    EffectRow* offset = add_row("Offset XY:");
    offset_x_val = offset->add_field(EFFECT_FIELD_DOUBLE);
    offset_x_val->id = "offsetx";
    offset_y_val = offset->add_field(EFFECT_FIELD_DOUBLE);
    offset_y_val->id = "offsety";

    prepend_text = add_row("Prepend:")->add_field(EFFECT_FIELD_STRING, 2);
    prepend_text->id = "prepend";

    connect(tc_select, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(scale_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(color_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(color_bg_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(bg_alpha, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(offset_x_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(offset_y_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(prepend_text, SIGNAL(changed()), this, SLOT(field_changed()));
}


void TimecodeEffect::redraw(double timecode) {

    if(qvariant_cast<bool>(tc_select->get_combo_data(timecode))){
        display_timecode = prepend_text->get_string_value(timecode) + frame_to_timecode(sequence->playhead, config.timecode_view, sequence->frame_rate);}
    else {
        double media_rate = parent_clip->getMediaFrameRate();
        display_timecode = prepend_text->get_string_value(timecode) + frame_to_timecode(timecode * media_rate, config.timecode_view, media_rate);}
    img.fill(Qt::transparent);

	QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
	int width = img.width();
	int height = img.height();

	// set font
	font.setStyleHint(QFont::Helvetica, QFont::PreferAntialias);
    font.setFamily("Helvetica");
    font.setPixelSize(qCeil(scale_val->get_double_value(timecode)*.01*(height/10)));
    p.setFont(font);
	QFontMetrics fm(font);

    QPainterPath path;

    int text_x, text_y, rect_y, offset_x, offset_y;
    int text_height = fm.height();
    int text_width = fm.width(display_timecode);
    QColor background_color = color_bg_val->get_color_value(timecode);
    int alpha_val = bg_alpha->get_double_value(timecode)*2.55;
    background_color.setAlpha(alpha_val);

    offset_x = int(offset_x_val->get_double_value(timecode));
    offset_y = int(offset_y_val->get_double_value(timecode));

    text_x = offset_x + (width/2) - (text_width/2);
    text_y = offset_y + height - height/10;
    rect_y = text_y + fm.descent()/2 - text_height;

    path.addText(text_x, text_y, font, display_timecode);

    p.setPen(Qt::NoPen);
    p.setBrush(background_color);
    p.drawRect(QRect(text_x-fm.descent()/2, rect_y, text_width+fm.descent(), text_height));
    p.setBrush(color_val->get_color_value(timecode));
    p.drawPath(path);
}
