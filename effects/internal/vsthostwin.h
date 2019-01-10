#ifndef VSTHOSTWIN_H
#define VSTHOSTWIN_H

#include "project/effect.h"

#include <vst/aeffectx.h>

// Plugin's dispatcher function
typedef VstIntPtr (*dispatcherFuncPtr)(AEffect *effect, VstInt32 opCode, VstInt32 index, VstInt32 value, void *ptr, float opt);

struct AEffect;
class QDialog;

class VSTHostWin : public Effect {
	Q_OBJECT
public:
	VSTHostWin(Clip* c, const EffectMeta* em);
	~VSTHostWin();
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	void custom_load(QXmlStreamReader& stream);
	void save(QXmlStreamWriter& stream);
private slots:
	void show_interface(bool show);
	void uncheck_show_button();
	void change_plugin();
private:
	EffectField* file_field;

	void loadPlugin();
	void freePlugin();
	dispatcherFuncPtr dispatcher;
	AEffect* plugin;
	bool configurePluginCallbacks();
	void startPlugin();
	void stopPlugin();
	void resumePlugin();
	void suspendPlugin();
	bool canPluginDo(char *canDoString);
	void initializeIO();
	void processAudio(long numFrames);
	float** inputs;
	float** outputs;
	QDialog* dialog;
	QPushButton* show_interface_btn;
	HMODULE modulePtr;
};

#endif // VSTHOSTWIN_H
