#ifndef VSTHOSTWIN_H
#define VSTHOSTWIN_H

#include "project/effect.h"

#include <vst/aeffectx.h>

// Plugin's dispatcher function
typedef VstIntPtr (*dispatcherFuncPtr)(AEffect *effect, VstInt32 opCode, VstInt32 index, VstInt32 value, void *ptr, float opt);

struct AEffect;

class VSTHostWin : public Effect {
	Q_OBJECT
public:
	VSTHostWin(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
private slots:
	void show_interface();
private:
	void loadPlugin();
	dispatcherFuncPtr dispatcher;
	AEffect* plugin;
	int configurePluginCallbacks();
	void startPlugin();
	void resumePlugin();
	void suspendPlugin();
	bool canPluginDo(char *canDoString);
	void initializeIO();
	void processAudio(long numFrames);
	void silenceChannel(float **channelData, int numChannels, long numFrames);
	float** inputs;
	float** outputs;
};

#endif // VSTHOSTWIN_H
