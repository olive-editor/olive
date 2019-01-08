#include "vsthostwin.h"

// adapted from http://teragonaudio.com/article/How-to-make-your-own-VST-host.html

#include <Windows.h>

#include <QPushButton>

#include "playback/audio.h"
#include "mainwindow.h"
#include "debug.h"

#define BLOCK_SIZE 512
#define CHANNEL_COUNT 2

// C callbacks
extern "C" {
	// Main host callback
	VstIntPtr VSTCALLBACK hostCallback(AEffect *effect, int opcode, int index, long long value, void *ptr, float opt) {
		switch(opcode) {
		case audioMasterVersion:
			return 2400;
		case audioMasterIdle:
			effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0);
			break;
		case audioMasterGetCurrentProcessLevel:
			return 0;
		// Handle other opcodes here... there will be lots of them
		default:
			dout << "[INFO] Plugin requested unhandled opcode" << opcode;
			break;
		}
	}
}

// Plugin's entry point
typedef AEffect *(*vstPluginFuncPtr)(audioMasterCallback host);
// Plugin's getParameter() method
typedef float (*getParameterFuncPtr)(AEffect *effect, VstInt32 index);
// Plugin's setParameter() method
typedef void (*setParameterFuncPtr)(AEffect *effect, VstInt32 index, float value);
// Plugin's processEvents() method
typedef VstInt32 (*processEventsFuncPtr)(VstEvents *events);
// Plugin's process() method
typedef void (*processFuncPtr)(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames);

void VSTHostWin::loadPlugin() {
	plugin = NULL;

	const char* vst_path = "C:\\Users\\Matt\\Downloads\\ToneGenerator_Win\\ToneGenerator_64b.dll";
	wchar_t win_char[200];
	mbstowcs(win_char, vst_path, 200);

	HMODULE modulePtr = LoadLibrary(win_char);
	if(modulePtr == NULL) {
		DWORD dll_err = GetLastError();
		dout << "[ERROR] Failed to load VST" << vst_path << "-" << dll_err;
		if (dll_err == 193) dout << "    Mixing 32-bit and 64-bit?";
		return;
	}

	vstPluginFuncPtr mainEntryPoint =
	(vstPluginFuncPtr)GetProcAddress(modulePtr, "VSTPluginMain");
	// Instantiate the plugin
	plugin = mainEntryPoint(hostCallback);
}

int VSTHostWin::configurePluginCallbacks() {
	// Check plugin's magic number
	// If incorrect, then the file either was not loaded properly, is not a
	// real VST plugin, or is otherwise corrupt.
	if(plugin->magic != kEffectMagic) {
		printf("Plugin's magic number is bad\n");
		return -1;
	}

	// Create dispatcher handle
	dispatcher = (dispatcherFuncPtr)(plugin->dispatcher);

	// Set up plugin callback functions
	plugin->getParameter = (getParameterFuncPtr)plugin->getParameter;
	plugin->processReplacing = (processFuncPtr)plugin->processReplacing;
	plugin->setParameter = (setParameterFuncPtr)plugin->setParameter;

	//return plugin;
	return 0;
}

void VSTHostWin::startPlugin() {
	dispatcher(plugin, effOpen, 0, 0, NULL, 0.0f);

	// Set some default properties
	dispatcher(plugin, effSetSampleRate, 0, 0, NULL, current_audio_freq());
	dispatcher(plugin, effSetBlockSize, 0, BLOCK_SIZE, NULL, 0.0f);

	resumePlugin();
}

void VSTHostWin::resumePlugin() {
	dispatcher(plugin, effMainsChanged, 0, 1, NULL, 0.0f);
}

void VSTHostWin::suspendPlugin() {
	dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0.0f);
}

bool VSTHostWin::canPluginDo(char *canDoString) {
	return (dispatcher(plugin, effCanDo, 0, 0, (void*)canDoString, 0.0f) > 0);
}

void VSTHostWin::initializeIO() {
	// inputs and outputs are assumed to be float** and are declared elsewhere,
	// most likely the are fields owned by this class. numChannels and blocksize
	// are also fields, both should be size_t (or unsigned int, if you prefer).
	inputs = new float* [CHANNEL_COUNT];
	outputs = new float* [CHANNEL_COUNT];
	for(int channel = 0; channel < CHANNEL_COUNT; channel++) {
		inputs[channel] = new float[BLOCK_SIZE];
		outputs[channel] = new float[BLOCK_SIZE];
	}
}

void VSTHostWin::processAudio(long numFrames) {
	// Always reset the output array before processing.
	silenceChannel(outputs, CHANNEL_COUNT, numFrames);

	plugin->processReplacing(plugin, inputs, outputs, numFrames);
}

void VSTHostWin::silenceChannel(float **channelData, int numChannels, long numFrames) {
	for(int channel = 0; channel < numChannels; ++channel) {
		for(long frame = 0; frame < numFrames; ++frame) {
		  channelData[channel][frame] = 0.0f;
		}
	}
}

VSTHostWin::VSTHostWin(Clip* c, const EffectMeta *em) : Effect(c, em) {
	EffectRow* interface_row = add_row("Open Interface");
	QPushButton* show_interface_btn = new QPushButton();
	interface_row->add_widget(show_interface_btn);
	loadPlugin();
	if (plugin != NULL) {
		initializeIO();
		configurePluginCallbacks();
		startPlugin();
	}
}

void VSTHostWin::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
	if (plugin != NULL) {
		// convert to floats
		int nb_frames = qMin(nb_bytes >> 2, BLOCK_SIZE);
		int lim = qMin(nb_bytes, BLOCK_SIZE << 2);
		for (int i=0;i<lim;i+=4) {
			qint16 left_sample = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
			qint16 right_sample = (qint16) (((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));

			int index = i >> 2;
			inputs[0][index] = float(left_sample) / float(INT16_MAX);
			inputs[1][index] = float(right_sample) / float(INT16_MAX);
		}

		// send to VST
		processAudio(nb_frames);

		// convert back to int16
		for (int i=0;i<nb_frames;i++) {
			qint16 left_sample = qRound(inputs[0][i]*INT16_MAX);
			qint16 right_sample = qRound(inputs[1][i]*INT16_MAX);

			int index = i << 2;

			samples[index+3] = (quint8) (right_sample >> 8);
			samples[index+2] = (quint8) right_sample;
			samples[index+1] = (quint8) (left_sample >> 8);
			samples[index] = (quint8) left_sample;
		}
	}
}

void VSTHostWin::show_interface() {
//	dispatcher(plugin, effEditOpen, 0, 1, NULL, 0.0f);
//	dispatcher(plugin, effEditOpen, 0, 0, mainWindow->winId());
}
