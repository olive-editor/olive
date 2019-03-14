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
};

#endif // RICHTEXTEFFECT_H
