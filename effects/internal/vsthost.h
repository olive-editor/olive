#ifndef VSTHOSTWIN_H
#define VSTHOSTWIN_H

#ifdef _WIN32

#include "project/effect.h"

#include "io/crossplatformlib.h"

#include <vst/aeffectx.h>

// Plugin's dispatcher function
typedef VstIntPtr (*dispatcherFuncPtr)(AEffect *effect, VstInt32 opCode, VstInt32 index, VstInt32 value, void *ptr, float opt);

struct AEffect;
class QDialog;

class VSTHost : public Effect {
	Q_OBJECT
public:
	VSTHost(Clip* c, const EffectMeta* em);
	~VSTHost();
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
	void processAudio(long numFrames);
	float** inputs;
	float** outputs;
	QDialog* dialog;
	QPushButton* show_interface_btn;
	ModulePtr modulePtr;
};

#endif

#endif // VSTHOSTWIN_H
