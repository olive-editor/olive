#ifndef FILLLEFTRIGHTEFFECT_H
#define FILLLEFTRIGHTEFFECT_H

#include "project/effect.h"

class FillLeftRightEffect : public Effect {
public:
	FillLeftRightEffect(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
private:
	EffectField* fill_type;
};

#endif // FILLLEFTRIGHTEFFECT_H
