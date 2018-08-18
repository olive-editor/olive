#ifndef TEXTEFFECT_H
#define TEXTEFFECT_H

#include "../effect.h"

#include <QFont>

class TextEffect : public Effect {
	Q_OBJECT
public:
	TextEffect(Clip* c);
	void process_image(QImage& img);
	Effect* copy(Clip* c);

	EffectField* text_val;
	EffectField* size_val;
	EffectField* set_color_button;
	EffectField* set_font_combobox;
	EffectField* halign_field;
	EffectField* valign_field;
	EffectField* word_wrap_field;

	EffectField* outline_bool;
	EffectField* outline_width;
	EffectField* outline_color;

	EffectField* shadow_bool;
	EffectField* shadow_distance;
	EffectField* shadow_color;
	EffectField* shadow_softness;
	EffectField* shadow_opacity;
private slots:
	void outline_enable(bool);
	void shadow_enable(bool);
private:
	QFont font;
};

#endif // TEXTEFFECT_H
