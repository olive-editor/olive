#ifndef VOLUMEEFFECT_H
#define VOLUMEEFFECT_H

#include "project/effect.h"

class VolumeEffect : public Effect {
    Q_OBJECT
public:
	VolumeEffect(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	EffectField* volume_val;
};

#endif // VOLUMEEFFECT_H
