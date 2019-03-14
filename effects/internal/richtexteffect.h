#ifndef RICHTEXTEFFECT_H
#define RICHTEXTEFFECT_H

#include "effects/effect.h"

class RichTextEffect : public Effect {
  Q_OBJECT
public:
  RichTextEffect(Clip* c, const EffectMeta *em);
  void redraw(double timecode);
private:
  StringField* text_val;
  DoubleField* padding_field;
  DoubleField* position_x;
  DoubleField* position_y;
  ComboField* vertical_align;
};

#endif // RICHTEXTEFFECT_H
