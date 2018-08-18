#ifndef VOLUMEEFFECT_H
#define VOLUMEEFFECT_H

#include "../effect.h"

class VolumeEffect : public Effect {
public:
	VolumeEffect(Clip* c);
	void refresh();
	void process_audio(quint8* samples, int nb_bytes);
	Effect* copy(Clip* c);

	EffectField* volume_val;
};

#endif // VOLUMEEFFECT_H
