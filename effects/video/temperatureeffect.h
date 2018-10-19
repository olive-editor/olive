#ifndef TEMPERATUREEFFECT_H
#define TEMPERATUREEFFECT_H

#include "../effect.h"

class TemperatureEffect : public Effect {
    Q_OBJECT
public:
    TemperatureEffect(Clip* c);
    void process_shader(double timecode);
private:
    EffectField* temp_val;
};

#endif // TEMPERATUREEFFECT_H
