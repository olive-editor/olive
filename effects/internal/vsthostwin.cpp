#include "vsthostwin.h"

// adapted from http://teragonaudio.com/article/How-to-make-your-own-VST-host.html

#include <Windows.h>

#include <QPushButton>
#include <QDialog>

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
		case 6: // audioMasterWantMidi
			return 0;
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

	const char* vst_path = "C:\\Program Files\\VSTPlugins\\ReaPlugs\\reaeq-standalone.dll";
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
	EffectRow* interface_row = add_row("Interface");
	show_interface_btn = new QPushButton("Show");
	show_interface_btn->setCheckable(true);
	connect(show_interface_btn, SIGNAL(toggled(bool)), this, SLOT(show_interface(bool)));
	interface_row->add_widget(show_interface_btn);
	loadPlugin();
	if (plugin != NULL) {
		initializeIO();
		configurePluginCallbacks();
		startPlugin();
		dialog = new QDialog(mainWindow);
		dialog->setWindowTitle("VST Plugin");
		dialog->setAttribute(Qt::WA_NativeWindow, true);
		dialog->setWindowFlags(dialog->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
		connect(dialog, SIGNAL(finished(int)), this, SLOT(uncheck_show_button()));
		dispatcher(plugin, effEditOpen, 0, 0, reinterpret_cast<HWND>(dialog->winId()), 0);

		ERect* eRect = NULL;
		plugin->dispatcher(plugin, effEditGetRect, 0, 0, &eRect, 0);
		dialog->setFixedWidth(eRect->right);
		dialog->setFixedHeight(eRect->bottom);
	}
}

void VSTHostWin::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
	if (plugin != NULL) {
		int interval = BLOCK_SIZE*4;
		for (int i=0;i<nb_bytes;i+=interval) {
			int process_size = qMin(interval, nb_bytes - i);
			int lim = i + process_size;

			// convert to float
			for (int j=i;j<lim;j+=4) {
				qint16 left_sample = (qint16) (((samples[j+1] & 0xFF) << 8) | (samples[j] & 0xFF));
				qint16 right_sample = (qint16) (((samples[j+3] & 0xFF) << 8) | (samples[j+2] & 0xFF));

				/*left_sample = 0;
				right_sample = 0;

				samples[j+3] = (quint8) (right_sample >> 8);
				samples[j+2] = (quint8) right_sample;
				samples[j+1] = (quint8) (left_sample >> 8);
				samples[j] = (quint8) left_sample;*/

				int index = (j-i)>>2;
				inputs[0][index] = float(left_sample) / float(INT16_MAX);
				inputs[1][index] = float(right_sample) / float(INT16_MAX);
			}

			// send to VST
			processAudio(process_size>>2);

			// convert back to int16
			for (int j=i;j<lim;j+=4) {
				int index = (j-i)>>2;

				qint16 left_sample = qRound(outputs[0][index] * INT16_MAX);
				qint16 right_sample = qRound(outputs[1][index] * INT16_MAX);

				samples[j+3] = (quint8) (right_sample >> 8);
				samples[j+2] = (quint8) right_sample;
				samples[j+1] = (quint8) (left_sample >> 8);
				samples[j] = (quint8) left_sample;
			}
		}

	}
}

void VSTHostWin::show_interface(bool show) {
	dialog->setVisible(show);
}

void VSTHostWin::uncheck_show_button() {
	show_interface_btn->setChecked(false);
}
